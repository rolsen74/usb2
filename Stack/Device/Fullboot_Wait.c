
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

struct USB2_FullBootWaiter
{
	struct USB2_FullBootWaiter *Next;
	struct Task *Task;
	U32 Mask;
};

// --

SEC_CODE S32
USB2_FullBoot_Wait( struct USBBase *usbbase )
{
	struct ExecIFace *IExec;
	struct USB2_FullBootWaiter **link;
	struct USB2_FullBootWaiter waiter;
	enum USB2_FullBootState state;

	S32 signal;

	IExec = usbbase->usb_IExec;

	IExec->Disable();

	state = usbbase->usb_FullBootState;

	IExec->Enable();

	if ( state != USB2_FullBoot_Pending )
	{
		return ( state == USB2_FullBoot_Ready );
	}

	signal = IExec->AllocSignal( -1 );

	if ( signal < 0 )
	{
		return ( FALSE );
	}

	waiter.Mask = 1UL << signal;

	waiter.Task = IExec->FindTask( NULL );

	IExec->Disable();

	state = usbbase->usb_FullBootState;

	if ( state == USB2_FullBoot_Pending )
	{
		waiter.Next = usbbase->usb_FullBootWaiters;
		usbbase->usb_FullBootWaiters = & waiter;
	}

	IExec->Enable();

	if ( state == USB2_FullBoot_Pending )
	{
		IExec->Wait( waiter.Mask | SIGBREAKF_CTRL_C );

		IExec->Disable();

		state = usbbase->usb_FullBootState;

		// On Ctrl-C, remove the waiter before returning to OpenDevice.
		if ( state == USB2_FullBoot_Pending )
		{
			link = & usbbase->usb_FullBootWaiters;

			while ( *link != & waiter )
			{
				link = & (*link)->Next;
			}

			*link = waiter.Next;
		}

		IExec->Enable();
	}

	IExec->FreeSignal( signal );

	return ( state == USB2_FullBoot_Ready );
}

// --

SEC_CODE void
USB2_FullBoot_Complete( struct USBBase *usbbase, S32 ready )
{
	struct ExecIFace *IExec;
	struct USB2_FullBootWaiter *waiter;

	IExec = usbbase->usb_IExec;

	IExec->Disable();

	if ( usbbase->usb_FullBootState != USB2_FullBoot_Pending )
	{
		IExec->Enable();

		return;
	}

	usbbase->usb_FullBootState = ready ? USB2_FullBoot_Ready : USB2_FullBoot_Failed;

	while ( usbbase->usb_FullBootWaiters )
	{
		waiter = usbbase->usb_FullBootWaiters;
		usbbase->usb_FullBootWaiters = waiter->Next;

		IExec->Signal( waiter->Task, waiter->Mask );
	}

	IExec->Enable();

	#ifdef USB2_BIND_DEBUG
	IExec->DebugPrintF( "USB : Full Boot device opens %s\n", ready ? "released" : "failed" );
	#endif
}

// --
