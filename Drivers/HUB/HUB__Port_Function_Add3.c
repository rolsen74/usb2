
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

SEC_CODE struct RealFunctionNode *
HUB__Port_Function_Address( struct USBBase *usbbase, struct intern *in, U32 speed, U32 port, U32 *address )
{
	struct RealFunctionNode *fn;

	*address = 0;

	fn = FUNCTION_ALLOC( in->HCDNode, &in->ASync_Functions, speed, in->Function->fkt_Tier + 1 );

	if ( ! fn )
	{
		return ( NULL );
	}

	NODE_ADDTAIL( &in->FN_Header, &fn->fkt_DriverNode );

	fn->fkt_PortNr = port;
	fn->fkt_Parent = in->Function;
	fn->fkt_EnumEpoch = in->EnumEpoch;
	fn->fkt_EnumFenced = TRUE;

	*address = HCD_ADDR_OBTAIN( in->HCDNode, fn );

	if ( *address )
	{
		fn->fkt_Address = *address;
	}

	return ( fn );
}

// --

SEC_CODE U32
HUB__Port_Function_Configure( struct USBBase *usbbase, struct intern *in, struct RealFunctionNode *fn, U32 port, struct USB2_PortStatus *status )
{
	U32 published;
	S32 err;

	if ( ! HUB_Can_Enumerate( usbbase, in ) )
	{
		return ( FALSE );
	}

	if ( ! FUNCTION_INIT( fn, &in->ASync_Drivers ) )
	{
		usbbase->usb_IExec->DebugPrintF( "USB HUB: HCD %lu hub %lu port %lu descriptor initialization failed at address %lu\n",
			in->HCDNode->hn_HCDIndex, in->Function->fkt_Address, port, fn->fkt_Address );

		return ( FALSE );
	}

	// Budget this hub request independently of the child's descriptor reads.
	in->ControlDeadline = HUB_Uptime( usbbase ) + 500000;

	err = HUB__Port_Status_Get( usbbase, in, port, status );

	in->ControlDeadline = 0;

	if ( ( err != USB2Err_NoError ) ||
		( ! ( status->wPortStatus & PORTSTATF_Connection ) ) || ( status->wPortChange & PORTCHGF_Connection ) ||
		( ! HUB_Can_Enumerate( usbbase, in ) ) || ( FUNCTION_LOCK( fn ) != LSTAT_Okay ) )
	{
		usbbase->usb_IExec->DebugPrintF( "USB HUB: HCD %lu hub %lu port %lu post-init status rejected (%ld), status $%04lx change $%04lx\n",
			in->HCDNode->hn_HCDIndex, in->Function->fkt_Address, port, err,
			(U32) status->wPortStatus, (U32) status->wPortChange );

		return ( FALSE );
	}

	SEMAPHORE_OBTAIN( &usbbase->usb_LockSemaphore );

	published = ( in->HCDNode->hn_AcceptIO ) &&
		( in->EnumEpoch == in->HCDNode->hn_IOEpoch ) &&
		( fn->fkt_HCDGeneration == in->HCDNode->hn_Generation );

	if ( published )
	{
		fn->fkt_DriverASync = &in->ASync_Drivers;
		in->PortFunctions[port] = fn;
		fn->fkt_HUB_Data = in->HUB_ID;

		published = USB2_Function_Publish( usbbase, fn );

		if ( published )
		{
			fn->fkt_EnumFenced = FALSE;
		}
		else
		{
			fn->fkt_DriverASync = NULL;
			in->PortFunctions[port] = NULL;
			fn->fkt_HUB_Data = 0;
		}
	}

	SEMAPHORE_RELEASE( &usbbase->usb_LockSemaphore );

	if ( published )
	{
		FUNCTION_BIND( fn, fn->fkt_DriverASync );
	}
	else
	{
		FUNCTION_UNLOCK( fn );
	}

	return ( published );
}

// --

SEC_CODE void
HUB__Port_Function_Discard( struct USBBase *usbbase, struct intern *in, struct RealFunctionNode *fn )
{
	NODE_REMNODE( &in->FN_Header, &fn->fkt_DriverNode );

	if ( FUNCTION_FREE( fn ) == FSTAT_Locked )
	{
		NODE_ADDHEAD( &in->FN_Header, &fn->fkt_DriverNode );
	}
}

// --

SEC_CODE struct RealFunctionNode *
HUB__Port_Function_Add3( struct RealFunctionNode *parent UNUSED, struct USBBase *usbbase, struct intern *in, U32 speed, U32 port, struct USB2_PortStatus *status )
{
	struct RealFunctionNode *fn;

	U32 address;

	fn = HUB__Port_Function_Address( usbbase, in, speed, port, &address );

	if ( ( fn ) && ( address ) && ( HUB__Port_Function_SetAddr( usbbase, in, fn ) ) &&
		( HUB__Port_Function_Configure( usbbase, in, fn, port, status ) ) )
	{
		return ( fn );
	}

	if ( fn )
	{
		HUB__Port_Function_Discard( usbbase, in, fn );
	}

	return ( NULL );
}

// --
