
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HID.h"

// --

SEC_CODE static U32
StackDetached( struct USBBase *usbbase, struct HIDData *hd )
{
	struct USB2_NotifyMessage *msg;

	U32 detached;

	detached = FALSE;

	while ( ( msg = MSGPORT_GETMSG( &hd->Register->reg_MsgPort ) ) )
	{
		if ( ( msg->nm_Type == USBNT_Function ) && ( msg->nm_Command == USBNC_Detached ) )
		{
			detached = TRUE;
		}

		MSGPORT_REPLYMSG( msg );
	}

	return ( detached );
}

// --

SEC_CODE static struct HID_ReportNode *
InputWorker( struct HIDData *hd, struct RealRequest *ioreq )
{
	struct HID_ReportLayout *layout;

	U8 *data;
	U32 length;
	U32 id;

	data = ioreq->req_Public.io_Data;
	length = MIN( ioreq->req_Public.io_Actual, ioreq->req_Public.io_Length );

	if ( ( ! data ) || ( ! length ) )
	{
		return ( NULL );
	}

	if ( hd->Driver_Mode == HID_DMode_Boot )
	{
		return ( Header_Head( &hd->Reports ) );
	}

	id = ( hd->NumberedInput ) ? data[0] : 0;
	layout = hd->Layouts[id];

	if ( ( ! layout ) || ( (U64) length * 8 < layout->Bits[HID_Report_Input] ) )
	{
		return ( NULL );
	}

	return ( layout->Owner[HID_Report_Input] );
}

// --

SEC_CODE static void
StopReads( struct USBBase *usbbase, struct HIDData *hd, U8 *active )
{
	U32 index;

	for ( index = 0; index < HID_IOReqCount; index++ )
	{
		if ( active[index] )
		{
			IO_STOP( hd->Res_Interrupt->IORequests[index] );

			active[index] = FALSE;
		}
	}
}

// --

SEC_CODE static void
HID_Resume( struct USBBase *usbbase, PTR data, PTR unused UNUSED )
{
	struct USB2_DriverMessage *msg;
	struct HIDData *hd;

	S32 promoted;
	U32 result;

	hd = data;
	msg = hd->StartMsg;

	result = HID_Main( usbbase, hd, &promoted );

	HID_Free( usbbase, hd );

	MEM_FREEVEC( hd );

	USB2_DriverWorkerResult( usbbase, msg, result );
}

// --

SEC_CODE static S32
HID_Promote( struct USBBase *usbbase, struct HIDData *hd, U32 mask )
{
	struct USB2_WorkerHandoff handoff;

	MEM_SET( &handoff, 0, sizeof( handoff ) );

	handoff.Entry = HID_Resume;
	handoff.Data = hd;
	handoff.Ports[0] = &hd->Register->reg_MsgPort.ump_MsgPort;
	handoff.Ports[1] = &hd->Return_MsgPort.ump_MsgPort;
	handoff.Ports[2] = &hd->Timer_MsgPort.ump_MsgPort;
	handoff.Endpoints[0] = hd->Res_Interrupt;
	handoff.Endpoints[1] = hd->Res_Control;
	handoff.Owners[0] = &hd->TaskAdr;
	handoff.Owners[1] = &hd->ASync_Drivers.ua_Parent;
	handoff.Pending = mask;

	return ( USB2_PromoteWorker( usbbase, &handoff ) );
}

// --

SEC_CODE U32
HID_Main( struct USBBase *usbbase, struct HIDData *hd, S32 *promoted )
{
	struct HID_ReportNode *rn;
	struct USB2_IORequest *request;
	struct RealRequest *ioreq;
	struct ExecIFace *IExec;
	struct HIDTransferState *transfer;

	U32 index;
	U32 mask;
	U32 wait;
	U32 result;

	IExec = usbbase->usb_IExec;
	transfer = &hd->Transfer;
	*promoted = FALSE;
	result = TASK_Return_Okay;
	wait = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E | hd->Res_Interrupt->MsgPortBit |
		hd->Register->reg_Public.Stack_MsgPortBit | hd->Return_MsgPort.ump_Signal.sig_Signal_Mask |
		hd->Timer_MsgPort.ump_Signal.sig_Signal_Mask;

	mask = TASK_SETSIGNAL( 0, wait );

	while ( TRUE )
	{
		if ( ( StackDetached( usbbase, hd ) ) || ( mask & SIGBREAKF_CTRL_C ) )
		{
			break;
		}

		if ( ( mask & SIGBREAKF_CTRL_E ) && ( HID_Promote( usbbase, hd, mask ) ) )
		{
			*promoted = TRUE;

			return ( TASK_Return_Okay );
		}

		while ( ( request = MSGPORT_GETMSG( &hd->Return_MsgPort ) ) )
		{
			for ( index = 0; index < HID_IOReqCount; index++ )
			{
				if ( hd->Res_Interrupt->IORequests[index] == request )
				{
					transfer->Forwarded[index] = FALSE;
					break;
				}
			}
		}

		while ( ( ioreq = (PTR) IExec->GetMsg( hd->Res_Interrupt->MsgPort ) ) )
		{
			for ( index = 0; index < HID_IOReqCount; index++ )
			{
				if ( hd->Res_Interrupt->IORequests[index] == &ioreq->req_Public )
				{
					break;
				}
			}

			if ( index == HID_IOReqCount )
			{
				continue;
			}

			transfer->Active[index] = FALSE;

			if ( ioreq->req_Public.io_Error )
			{
				transfer->Recovery = ioreq->req_Public.io_Error;
				break;
			}

			transfer->Errors = 0;

			rn = InputWorker( hd, ioreq );

			if ( ( rn ) && ( rn->Running ) )
			{
				transfer->Forwarded[index] = TRUE;

				MSGPORT_PUTMSG( &rn->IOReq_MsgPort, ioreq );
			}
		}

		if ( transfer->Recovery == USB2Err_Device_Detached )
		{
			break;
		}

		if ( transfer->Recovery )
		{
			if ( ! hd->Timer_Added )
			{
				StopReads( usbbase, hd, transfer->Active );

				transfer->Errors++;

				if ( transfer->Errors >= 20 )
				{
					USBDEBUG( "HID: interrupt endpoint stopped after error %ld", transfer->Recovery );

					result = TASK_Return_Error;
					break;
				}

				hd->Timer_IOReq.Time.Seconds = 0;
				hd->Timer_IOReq.Time.Microseconds = ( transfer->Recovery == USB2Err_Host_NoBandWidth ) ? 250000 : 50000;
				hd->Timer_IOReq.Request.io_Command = TR_ADDREQUEST;
				hd->Timer_Added = TRUE;

				IO_SEND( &hd->Timer_IOReq );
			}
			else if ( MSGPORT_GETMSG( &hd->Timer_MsgPort ) )
			{
				hd->Timer_Added = FALSE;

				if ( ( transfer->Recovery != USB2Err_Host_Stall ) || ( ENDPOINTRES_DESTALL( hd->Res_Interrupt ) ) )
				{
					transfer->Recovery = 0;
				}
				else
				{
					// Run the delayed recovery again without submitting reads.
					mask = 0;
					continue;
				}
			}
		}

		if ( ! transfer->Recovery )
		{
			for ( index = 0; index < HID_IOReqCount; index++ )
			{
				if ( ( ! transfer->Active[index] ) && ( ! transfer->Forwarded[index] ) )
				{
					transfer->Active[index] = TRUE;

					IO_SEND( hd->Res_Interrupt->IORequests[index] );
				}
			}
		}

		mask = TASK_WAIT( wait );
	}

	if ( hd->Timer_Added )
	{
		IO_STOP( &hd->Timer_IOReq );

		hd->Timer_Added = FALSE;
	}

	StopReads( usbbase, hd, transfer->Active );

	return ( result );
}

// --
