
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

SEC_CODE S32 HUB__Port_Function_Add( struct USBBase *usbbase, struct intern *in, U32 port )
{
struct RealFunctionNode *fn;
struct USB2_PortStatus stat;
S32 retval;
U32 retry;
S32 err;

	retval = FALSE;

	fn = in->Function;

//	usbbase->usb_IExec->Disable();
//	usbbase->usb_IExec->DebugPrintF( "\nAdd Deivce (Enter)\n" );
//	usbbase->usb_IExec->DebugPrintF( "HUB Tier    : %lu\n", fn->fkt_Tier );
//	usbbase->usb_IExec->DebugPrintF( "Port        : %lu\n", port );
//	usbbase->usb_IExec->Enable();

	#ifdef DO_STABLE

	if (( port == 0 ) || ( port > in->Ports ))
	{
		USBDEBUG( "HUB__Port_Function_Add : Illegal Port Number (%lu)", port );
		goto bailout;
	}

	if ( in->PortFunctions[port] )
	{
		USBDEBUG( "HUB__Port_Function_Add : Warning Port %lu not empty", port );
	}

	#endif

	// --

	USBDEBUG( "HUB__Port_Function_Add : 1 : Get Status" );

	err = HUB__Port_Status_Get( usbbase, in, port, & stat );

	if ( err != USB2Err_NoError )
	{
		USBDEBUG( "HUB__Port_Function_Add : Error reading Port Status (%ld)", err );
		goto bailout;
	}

	USBDEBUG( "HUB__Port_Function_Add   :  2 : Port Stat $%04lx:%04lx", stat.wPortChange, stat.wPortStatus );

	// --

	if ( stat.wPortStatus & PORTSTATF_Enable )
	{
		USBDEBUG( "HUB__Port_Function_Add : 3 : Clr Port Feature( PORT_ENABLE )" );

		err = HUB__Port_Feature_Clr( usbbase, in, port, PORT_ENABLE );

		if ( err != USB2Err_NoError )
		{
			USBDEBUG( "HUB__Port_Function_Add : Error clearing Port Enable (%ld)", err );
			goto bailout;
		}
	}

	if (( stat.wPortStatus & PORTSTATF_Connection ) == 0 )
	{
		USBDEBUG( "HUB__Port_Function_Add : No Device connected" );
		goto bailout;
	}

	// --

	for( retry=0 ; retry<5 ; retry++ )
	{
		HUB__Port_FAdd_Retry( fn, & stat, usbbase, in, retry, port );

		if ( in->PortFunctions[port] )
		{
			retval = TRUE;
			break;
		}

		if ( ( ! ( stat.wPortStatus & PORTSTATF_Connection ) ) || ( stat.wPortChange & PORTCHGF_Connection ) )
		{
			break;
		}
	}

	// --

bailout:

//	usbbase->usb_IExec->Disable();
//	usbbase->usb_IExec->DebugPrintF( "\nAdd Deivce (Leave)\n" );
//	usbbase->usb_IExec->DebugPrintF( "HUB Tier    : %lu\n", fn->fkt_Tier );
//	usbbase->usb_IExec->DebugPrintF( "Port        : %lu\n", port );
//	usbbase->usb_IExec->DebugPrintF( "retval      : %lu\n", retval );
//	usbbase->usb_IExec->Enable();

	return( retval );
}

// --

SEC_CODE enum HUB_AttachResult
HUB__Port_Attach( struct USBBase *usbbase, struct intern *in, U32 port )
{
	struct RealFunctionNode *fn;
	struct USB2_HCDNode *hn;
	struct USB2_PortStatus status;

	enum HUB_AttachResult result;
	U32 address;
	U32 speed;
	U32 addressed;
	U32 held;
	S32 err;

	hn = in->HCDNode;
	fn = NULL;
	address = 0;
	addressed = FALSE;
	held = FALSE;
	result = HUB_Attach_Retry;
	status.wPortStatus = 0;
	status.wPortChange = 0;

	if ( ! HUB_Can_Enumerate( usbbase, in ) )
	{
		return ( HUB_Attach_Deferred );
	}

	if ( ! SEMAPHORE_ATTEMPT( &hn->hn_AddressZero ) )
	{
		return ( HUB_Attach_Deferred );
	}

	SEMAPHORE_OBTAIN( &usbbase->usb_Addr_Semaphore );

	held = ( hn->hn_AddressHold[0].Hub[0] != 0 );

	SEMAPHORE_RELEASE( &usbbase->usb_Addr_Semaphore );

	if ( held )
	{
		result = HUB_Attach_Deferred;
		goto unlock;
	}

	SEMAPHORE_OBTAIN( &usbbase->usb_LockSemaphore );

	in->EnumEpoch = hn->hn_IOEpoch;

	SEMAPHORE_RELEASE( &usbbase->usb_LockSemaphore );

	in->ControlDeadline = HUB_Uptime( usbbase ) + 500000;
	err = HUB__Port_Status_Get( usbbase, in, port, &status );

	if ( err )
	{
		goto unlock;
	}

	if ( status.wPortChange & PORTCHGF_Connection )
	{
		result = HUB_Attach_Reconnect;
		goto unlock;
	}

	if ( ! ( status.wPortStatus & PORTSTATF_Connection ) )
	{
		result = HUB_Attach_Gone;
		goto unlock;
	}

	if ( status.wPortStatus & ( PORTSTATF_Enable | PORTSTATF_Reset ) )
	{
		if ( ! HUB__Port_Isolate( usbbase, in, port ) )
		{
			HUB__Port_Hold( usbbase, in, port, 0 );
			goto unlock;
		}
	}

	err = HUB__Port_Reset( usbbase, in, port, &status );

	if ( ! err )
	{
		speed = USBSPEED_Full;

		if ( status.wPortStatus & PORTSTATF_Low_Speed )
		{
			speed = USBSPEED_Low;
		}
		else if ( status.wPortStatus & PORTSTATF_High_Speed )
		{
			speed = USBSPEED_High;
		}

		fn = HUB__Port_Function_Address( usbbase, in, MIN( speed, in->Function->fkt_Speed ), port, &address );

		if ( ( fn ) && ( address ) )
		{
			in->ControlDeadline = HUB_Uptime( usbbase ) + 1000000;
			addressed = HUB__Port_Function_SetAddr( usbbase, in, fn ) && HUB_Can_Enumerate( usbbase, in );
		}
	}

	if ( ( err ) && ( err != USB2Err_Device_OwnershipChange ) &&
		( ! ( status.wPortChange & PORTCHGF_Connection ) ) )
	{
		usbbase->usb_IExec->DebugPrintF( "USB HUB: HCD %lu hub %lu port %lu reset failed (%ld), status $%04lx change $%04lx\n",
			hn->hn_HCDIndex, in->Function->fkt_Address, port, err,
			(U32) status.wPortStatus, (U32) status.wPortChange );
	}

	if ( ! addressed )
	{
		in->ControlDeadline = HUB_Uptime( usbbase ) + 500000;

		if ( ! HUB__Port_Isolate( usbbase, in, port ) )
		{
			HUB__Port_Hold( usbbase, in, port, 0 );

			if ( address )
			{
				HUB__Port_Hold( usbbase, in, port, address );
			}
		}
		else if ( ( err == USB2Err_Device_OwnershipChange ) &&
			( HUB__Port_Status_Get( usbbase, in, port, &status ) == USB2Err_NoError ) &&
			( ! ( status.wPortStatus & PORTSTATF_Connection ) ) )
		{
			result = HUB_Attach_Gone;
		}
		else if ( status.wPortChange & PORTCHGF_Connection )
		{
			result = HUB_Attach_Reconnect;
		}
	}

unlock:
	in->ControlDeadline = 0;

	SEMAPHORE_RELEASE( &hn->hn_AddressZero );

	if ( addressed )
	{
		if ( HUB__Port_Function_Configure( usbbase, in, fn, port, &status ) )
		{
			return ( HUB_Attach_Complete );
		}

		in->ControlDeadline = HUB_Uptime( usbbase ) + 500000;

		if ( ! HUB__Port_Isolate( usbbase, in, port ) )
		{
			HUB__Port_Hold( usbbase, in, port, address );
		}

		in->ControlDeadline = 0;
	}

	if ( fn )
	{
		HUB__Port_Function_Discard( usbbase, in, fn );
	}

	return ( result );
}

// --
