
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// --

#include "usb2_all.h"

#include "HUB.h"

// --

static SEC_CODE S32
WaitForReconnectedPort( struct USBBase *usbbase, struct intern *in, U32 port )
{
	struct USB2_PortStatus stat;

	U32 waited;
	U32 stable;

	stable = 0;

	// A disconnect and reconnect can both occur before the hub is polled.
	for ( waited = 0; waited < 500; waited += 25 )
	{
		HUB_DELAY_MS( 25 );

		if ( HUB__Port_Status_Get( usbbase, in, port, & stat ) != USB2Err_NoError )
		{
			return ( FALSE );
		}

		if ( stat.wPortChange & PORTCHGF_Connection )
		{
			if ( HUB__Port_Feature_Clr( usbbase, in, port, PORT_CLR_CONNECTION ) != USB2Err_NoError )
			{
				return ( FALSE );
			}

			stable = 0;
		}
		else if ( stat.wPortStatus & PORTSTATF_Connection )
		{
			stable += 25;

			if ( stable >= 100 )
			{
				return ( TRUE );
			}
		}
		else
		{
			stable = 0;
		}
	}

	return ( FALSE );
}

// --

SEC_CODE void HUB__Port_Chg( struct USBBase *usbbase, struct intern *in, U32 port )
{
struct RealFunctionNode *fn;
struct USB2_HCDNode *hn;
struct USB2_PortStatus stat;

S32 err;
S32 reconnect;
S32 recover;

	// --

	#if 0
	usbbase->usb_IExec->Disable();
	usbbase->usb_IExec->DebugPrintF( "\nHUB : Handle Port Change\n" );
	usbbase->usb_IExec->DebugPrintF( "HUB Tier    : %lu\n", in->Function->fkt_Tier );
	usbbase->usb_IExec->DebugPrintF( "Port        : %lu\n", port );
	usbbase->usb_IExec->Enable();
	#endif

	// --

	#ifdef DO_STABLE

	if (( port == 0 ) || ( port > in->Ports ))
	{
		USBERROR( "HUB__Port_Chg : Illegal Port Number (%lu)", port );
		goto bailout;
	}

	#endif

	// -- Get Status

	USBDEBUG( "HUB__Port_Chg : Handle Port Change for Port %lu", port );

	err = HUB__Port_Status_Get( usbbase, in, port, & stat );

	if ( err != USB2Err_NoError )
	{
		USBERROR( "HUB__Port_Chg : Error reading Port Status (%ld)", err );
		goto bailout;
	}

	USBDEBUG( "Port Stat $%04lx:%04lx", stat.wPortChange, stat.wPortStatus );

	// -- Handle Port Change

	hn = in->Function->fkt_HCD;
	fn = in->PortFunctions[port];

	SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

	recover = ( fn ) && ( fn->fkt_PortRecovery == PORT_RECOVERY_READY ) && ( hn->hn_AcceptIO )
		&& ( ! in->Function->fkt_Detach ) && ( fn->fkt_HCDGeneration == hn->hn_Generation );

	SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

	if ( ( recover ) || ( stat.wPortChange & PORTCHGF_Connection ) )
	{
		#ifdef DO_INFO
		usbbase->usb_IExec->Disable();
		usbbase->usb_IExec->DebugPrintF( "\nHUB : Clearing Connect Change\n" );
		usbbase->usb_IExec->DebugPrintF( "HUB Tier    : %lu\n", in->Function->fkt_Tier );
		usbbase->usb_IExec->DebugPrintF( "Port        : %ld\n", port );
		usbbase->usb_IExec->Enable();
		#endif

		if ( stat.wPortChange & PORTCHGF_Connection )
		{
			err = HUB__Port_Feature_Clr( usbbase, in, port, PORT_CLR_CONNECTION );

			if ( err != USB2Err_NoError )
			{
				USBERROR( "HUB__Port_Chg : Error clearing Port Connect (%ld)", err );
				goto bailout;
			}
		}

		reconnect = ( fn ) && ( stat.wPortStatus & PORTSTATF_Connection ) &&
			( in->Function->fkt_Speed < USBSPEED_Super );

		if ( fn )
		{
			usbbase->usb_IExec->DebugPrintF(
				"USB HUB: tier %lu addr %lu port %lu removing addr %lu (%04lx:%04lx), status $%04lx change $%04lx\n",
				in->Function->fkt_Tier, in->Function->fkt_Address, port, fn->fkt_Address,
				(U32) ( fn->fkt_DeviceDescriptor ? LE_SWAP16( fn->fkt_DeviceDescriptor->VendorID ) : 0 ),
				(U32) ( fn->fkt_DeviceDescriptor ? LE_SWAP16( fn->fkt_DeviceDescriptor->DeviceID ) : 0 ),
				(U32) stat.wPortStatus, (U32) stat.wPortChange );

			HUB__Port_Function_Rem( usbbase, in, port );
		}

		if ( stat.wPortStatus & PORTSTATF_Connection )
		{
			if ( ( reconnect ) && ( ! WaitForReconnectedPort( usbbase, in, port ) ) )
			{
				usbbase->usb_IExec->DebugPrintF( "USB HUB: tier %lu addr %lu port %lu reconnect did not stabilize\n",
					in->Function->fkt_Tier, in->Function->fkt_Address, port );

				goto bailout;
			}

			#ifdef DO_INFO
			usbbase->usb_IExec->Disable();
			usbbase->usb_IExec->DebugPrintF( "\nHUB : Adding Device\n" );
			usbbase->usb_IExec->DebugPrintF( "HUB Tier    : %lu\n", in->Function->fkt_Tier );
			usbbase->usb_IExec->DebugPrintF( "Port        : %lu\n", port );
			usbbase->usb_IExec->Enable();
			#endif

			if ( ! HUB__Port_Function_Add( usbbase, in, port ) )
			{
				USBERROR( "HUB__Port_Chg : HUB__Port_Function_Add() failed" );

				if ( reconnect )
				{
					usbbase->usb_IExec->DebugPrintF( "USB HUB: tier %lu addr %lu port %lu re-enumeration failed\n",
						in->Function->fkt_Tier, in->Function->fkt_Address, port );
				}
			}
			else if ( reconnect )
			{
				usbbase->usb_IExec->DebugPrintF( "USB HUB: tier %lu addr %lu port %lu re-enumerated as addr %lu\n",
					in->Function->fkt_Tier, in->Function->fkt_Address, port, in->PortFunctions[port]->fkt_Address );
			}
		}
	}

	// -- Handle Port Enable

	if ( stat.wPortChange & PORTCHGF_Enable )
	{
		#ifdef DO_INFO
		usbbase->usb_IExec->Disable();
		usbbase->usb_IExec->DebugPrintF( "\nHUB : Clearing Enable Change\n" );
		usbbase->usb_IExec->DebugPrintF( "HUB Tier    : %lu\n", in->Function->fkt_Tier );
		usbbase->usb_IExec->DebugPrintF( "Port        : %lu\n", port );
		usbbase->usb_IExec->Enable();
		#endif

		err = HUB__Port_Feature_Clr( usbbase, in, port, PORT_CLR_ENABLE );

		if ( err != USB2Err_NoError )
		{
			USBDEBUG( "Error clearing Port Enable (%ld)", err );
			goto bailout;
		}
	}

	// -- Handle Port Suspend

	if ( stat.wPortChange & PORTCHGF_Suspend )
	{
		#ifdef DO_INFO
		usbbase->usb_IExec->Disable();
		usbbase->usb_IExec->DebugPrintF( "\nHUB : Clearing Suspend Change\n" );
		usbbase->usb_IExec->DebugPrintF( "HUB Tier    : %lu\n", in->Function->fkt_Tier );
		usbbase->usb_IExec->DebugPrintF( "Port        : %lu\n", port );
		usbbase->usb_IExec->Enable();
		#endif

		err = HUB__Port_Feature_Clr( usbbase, in, port, PORT_CLR_SUSPEND );

		if ( err != USB2Err_NoError )
		{
			USBDEBUG( "Error clearing Port Suspend (%ld)", err );
			goto bailout;
		}
	}

	// -- Handle Port Over Current

	if ( stat.wPortChange & PORTCHGF_Over_Current )
	{
		#ifdef DO_INFO
		usbbase->usb_IExec->Disable();
		usbbase->usb_IExec->DebugPrintF( "\nHUB : Clearing OverCurrent Change\n" );
		usbbase->usb_IExec->DebugPrintF( "HUB Tier    : %lu\n", in->Function->fkt_Tier );
		usbbase->usb_IExec->DebugPrintF( "Port        : %lu\n", port );
		usbbase->usb_IExec->Enable();
		#endif

		err = HUB__Port_Feature_Clr( usbbase, in, port, PORT_CLR_OVER_CURRENT );

		if ( err != USB2Err_NoError )
		{
			USBDEBUG( "Error clearing Port OverCurrent (%ld)", err );
			goto bailout;
		}
	}

	// -- Handle Port Reset

	if ( stat.wPortChange & PORTCHGF_Reset )
	{
		#ifdef DO_INFO
		usbbase->usb_IExec->Disable();
		usbbase->usb_IExec->DebugPrintF( "\nHUB : Clearing Reset Change\n" );
		usbbase->usb_IExec->DebugPrintF( "HUB Tier    : %lu\n", in->Function->fkt_Tier );
		usbbase->usb_IExec->DebugPrintF( "Port        : %lu\n", port );
		usbbase->usb_IExec->Enable();
		#endif

		err = HUB__Port_Feature_Clr( usbbase, in, port, PORT_CLR_RESET );

		if ( err != USB2Err_NoError )
		{
			USBDEBUG( "Error clearing Port Reset (%ld)", err );
			goto bailout;
		}
	}

	if ( in->Function->fkt_Speed >= USBSPEED_Super )
	{
		if ( stat.wPortChange & PORTCHGF_BH_PORT_RESET )
		{
			err = HUB__Port_Feature_Clr( usbbase, in, port, PORT_CLR_BH_RESET );

			if ( err != USB2Err_NoError )
			{
				goto bailout;
			}
		}

		if ( stat.wPortChange & PORTCHGF_PORT_LINK_STATE )
		{
			err = HUB__Port_Feature_Clr( usbbase, in, port, PORT_CLR_LINK_STATE );

			if ( err != USB2Err_NoError )
			{
				goto bailout;
			}
		}

		if ( stat.wPortChange & PORTCHGF_PORT_CONFIG_ERROR )
		{
			err = HUB__Port_Feature_Clr( usbbase, in, port, PORT_CLR_CONFIG_ERROR );

			if ( err != USB2Err_NoError )
			{
				goto bailout;
			}
		}
	}

	// --

bailout:

	return;
}
