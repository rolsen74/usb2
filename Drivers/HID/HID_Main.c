
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

SEC_CODE U32
HID_Main( struct USBBase *usbbase, struct HIDData *hd )
{
	struct HID_ReportNode *rn;
	struct USB2_IORequest *request;
	struct RealRequest *ioreq;
	struct ExecIFace *IExec;

	U32 index;
	U32 errors;
	U32 mask;
	U32 wait;
	U32 result;
	S32 recovery;
	U8 active[HID_IOReqCount];
	U8 forwarded[HID_IOReqCount];

	IExec = usbbase->usb_IExec;
	errors = 0;
	recovery = 0;
	result = TASK_Return_Okay;
	wait = SIGBREAKF_CTRL_C | hd->Res_Interrupt->MsgPortBit |
		hd->Register->reg_Public.Stack_MsgPortBit | hd->Return_MsgPort.ump_Signal.sig_Signal_Mask |
		hd->Timer_MsgPort.ump_Signal.sig_Signal_Mask;

	MEM_SET( active, 0, sizeof( active ) );

	MEM_SET( forwarded, 0, sizeof( forwarded ) );

	mask = TASK_SETSIGNAL( 0, wait );

	while ( TRUE )
	{
		if ( ( StackDetached( usbbase, hd ) ) || ( mask & SIGBREAKF_CTRL_C ) )
		{
			break;
		}

		while ( ( request = MSGPORT_GETMSG( &hd->Return_MsgPort ) ) )
		{
			for ( index = 0; index < HID_IOReqCount; index++ )
			{
				if ( hd->Res_Interrupt->IORequests[index] == request )
				{
					forwarded[index] = FALSE;
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

			active[index] = FALSE;

			if ( ioreq->req_Public.io_Error )
			{
				recovery = ioreq->req_Public.io_Error;
				break;
			}

			errors = 0;

			rn = InputWorker( hd, ioreq );

			if ( ( rn ) && ( rn->Running ) )
			{
				forwarded[index] = TRUE;

				MSGPORT_PUTMSG( &rn->IOReq_MsgPort, ioreq );
			}
		}

		if ( recovery == USB2Err_Device_Detached )
		{
			break;
		}

		if ( recovery )
		{
			if ( ! hd->Timer_Added )
			{
				StopReads( usbbase, hd, active );

				errors++;

				if ( errors >= 20 )
				{
					USBDEBUG( "HID: interrupt endpoint stopped after error %ld", recovery );

					result = TASK_Return_Error;
					break;
				}

				hd->Timer_IOReq.Time.Seconds = 0;
				hd->Timer_IOReq.Time.Microseconds = ( recovery == USB2Err_Host_NoBandWidth ) ? 250000 : 50000;
				hd->Timer_IOReq.Request.io_Command = TR_ADDREQUEST;
				hd->Timer_Added = TRUE;

				IO_SEND( &hd->Timer_IOReq );
			}
			else if ( MSGPORT_GETMSG( &hd->Timer_MsgPort ) )
			{
				hd->Timer_Added = FALSE;

				if ( ( recovery != USB2Err_Host_Stall ) || ( ENDPOINTRES_DESTALL( hd->Res_Interrupt ) ) )
				{
					recovery = 0;
				}
				else
				{
					// Run the delayed recovery again without submitting reads.
					mask = 0;
					continue;
				}
			}
		}

		if ( ! recovery )
		{
			for ( index = 0; index < HID_IOReqCount; index++ )
			{
				if ( ( ! active[index] ) && ( ! forwarded[index] ) )
				{
					active[index] = TRUE;

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

	StopReads( usbbase, hd, active );

	return ( result );
}

// --
