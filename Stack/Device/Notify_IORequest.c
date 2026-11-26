
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

// Leave room for topology notifications when the collector falls behind.
#define IO_NOTIFY_LIMIT 64

// --

SEC_CODE static S32
IO_Filter_Matches( const struct USB2_NotifyNode *node, const struct RealFunctionNode *fn, USB2_ID interface )
{
	const struct USB2_IOCaptureTarget *target;

	USB2_ID hcd;
	U32 index;
	S32 enabled;
	S32 deviceRule;

	hcd = fn->fkt_HCD ? fn->fkt_HCD->hn_NotifyID : 0;
	enabled = FALSE;
	deviceRule = FALSE;

	for( index = 0; index < node->nn_IOFilterCount; index++ )
	{
		target = &node->nn_IOFilter[index];

		if ( ( interface ) && ( target->ID == interface ) )
		{
			return ( target->Enabled != 0 );
		}

		if ( ( fn->fkt_NotifyID ) && ( target->ID == fn->fkt_NotifyID ) )
		{
			enabled = target->Enabled != 0;
			deviceRule = TRUE;
		}
		else if ( ( ! deviceRule ) && ( hcd ) && ( target->ID == hcd ) )
		{
			enabled = target->Enabled != 0;
		}
	}

	return ( enabled );
}

// --

// The caller holds the stack lock and still owns the completed request.
SEC_CODE void
USB2_Notify_IORequest( struct USBBase *usbbase, const struct RealRequest *ioreq )
{
	struct USB2_NotifyMessage *msg;
	struct USB2_NotifyNode *node;
	const struct RealFunctionNode *fn;
	const struct USB2_InterfaceHeader *interface;

	USB2_ID id;
	U32 sequence;

	if ( ! usbbase->usb_Notify_ReplyMsgPort )
	{
		return;
	}

	fn = ioreq->req_Function;
	sequence = 0;

	interface = USB2_IORequest_Interface( ioreq );

	id = interface ? interface->ih_NotifyID : 0;

	SEMAPHORE_OBTAIN( &usbbase->usb_Notify_Semaphore );

	for( node = usbbase->usb_Notify_Header.uh_Head; node; node = Node_Next( node ) )
	{
		if ( usbbase->usb_Notify_IOPending >= IO_NOTIFY_LIMIT )
		{
			break;
		}

		if ( ( node->nn_Type != USBNT_IORequest ) || ( ! IO_Filter_Matches( node, fn, id ) ) )
		{
			continue;
		}

		if ( ! sequence )
		{
			sequence = ++usbbase->usb_Notify_IOSequence;
		}

		msg = MEMORY_ALLOC( MEMID_NotifyMessage, TRUE );

		if ( ! msg )
		{
			break;
		}

		msg->nm_Message.mn_ReplyPort = &usbbase->usb_Notify_ReplyMsgPort->ump_MsgPort;
		msg->nm_Message.mn_Length = sizeof( *msg );
		msg->nm_Type = USBNT_IORequest;
		msg->nm_Command = USBNC_Completed;
		msg->nm_ID = fn->fkt_NotifyID;

		USB2_IORequest_Describe( ioreq, interface, &msg->nm_Data.nm_IORequest );

		msg->nm_Data.nm_IORequest.nm_Sequence = sequence;
		msg->nm_Data.nm_IORequest.nm_Actual = ioreq->req_Public.io_Actual;
		msg->nm_Data.nm_IORequest.nm_Error = ioreq->req_Public.io_Error;
		usbbase->usb_Notify_IOPending++;

		usbbase->usb_IExec->PutMsg( node->nn_MsgPort, &msg->nm_Message );
	}

	SEMAPHORE_RELEASE( &usbbase->usb_Notify_Semaphore );
}

// --
