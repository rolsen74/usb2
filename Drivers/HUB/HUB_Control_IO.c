/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HUB.h"

// --

SEC_CODE U64
HUB_Uptime( struct USBBase *usbbase )
{
	struct TimeVal time;

	usbbase->usb_ITimer->GetUpTime( &time );

	return ( (U64) time.Seconds * 1000000 + time.Microseconds );
}

// --

SEC_CODE U32
HUB_Can_Enumerate( struct USBBase *usbbase, struct intern *in )
{
	struct RealFunctionNode *fn;
	struct USB2_HCDNode *hn;

	U32 active;

	fn = in->Function;
	hn = in->HCDNode;

	if ( TASK_SETSIGNAL( 0, 0 ) & SIGBREAKF_CTRL_C )
	{
		return ( FALSE );
	}

	SEMAPHORE_OBTAIN( &usbbase->usb_LockSemaphore );

	active = ( hn->hn_AcceptIO ) && ( ! fn->fkt_Detach ) && ( ! fn->fkt_FreeMe ) &&
		( ( ! fn->fkt_Tier ) || ( fn->fkt_HCDGeneration == hn->hn_Generation ) ) &&
		( ( ! in->ControlDeadline ) || ( in->EnumEpoch == hn->hn_IOEpoch ) );

	SEMAPHORE_RELEASE( &usbbase->usb_LockSemaphore );

	return ( active );
}

// --

SEC_CODE S32
HUB_Control_IO( struct USBBase *usbbase, struct intern *in, struct USB2_IORequest *request )
{
	struct RealRequest *real;

	U64 now;
	U32 timeout;

	real = (PTR) request;
	timeout = request->io_TimeOut;

	if ( in->ControlDeadline )
	{
		now = HUB_Uptime( usbbase );

		if ( now >= in->ControlDeadline )
		{
			return ( USB2Err_Host_Timeout );
		}

		if ( ! HUB_Can_Enumerate( usbbase, in ) )
		{
			return ( USB2Err_Device_Detached );
		}

		request->io_TimeOut = MIN( timeout ? timeout : 1000000, in->ControlDeadline - now );
		real->req_ExpectedEpoch = in->EnumEpoch;
		real->req_CheckEpoch = TRUE;
	}

	IO_DO( request );

	real->req_CheckEpoch = FALSE;
	request->io_TimeOut = timeout;

	return ( request->io_Error );
}

// --
