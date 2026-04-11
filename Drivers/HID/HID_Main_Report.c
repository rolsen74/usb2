
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

SEC_CODE static void __Stack( struct USBBase *usbbase, struct HIDData *hd )
{
struct USB2_NotifyMessage *msg;
S32 stop;

	TASK_NAME_ENTER( "HID : __Stack" );

	stop = FALSE;

	while(( msg = MSGPORT_GETMSG( & hd->Register->reg_MsgPort )))
	{
		switch( msg->nm_Type )
		{

			default:
			{
				USBDEBUG( "HID : Unknown Message Type (%lu)", msg->nm_Command );
				break;
			}
		}

		MSGPORT_REPLYMSG( msg );
	}

	if ( stop )
	{

	}

	TASK_NAME_LEAVE();
}

// --

SEC_CODE static void __Interrupt( struct USBBase *usbbase, struct HIDData *hd )
{
struct HID_ReportNode *rn;
struct RealRequest *ioreq;
struct ExecIFace *IExec;
U32 reportid;
U32 reply;
S32 max;
U8 *data;

	TASK_NAME_ENTER( "__Interrupt" );

	IExec = usbbase->usb_IExec;

	max = HID_IOReqCount;

	while( TRUE )
	{
		if ( 0 > max-- )
		{
			break;
		}

		ioreq = (PTR) IExec->GetMsg( hd->Res_Interrupt->MsgPort );

		if ( ! ioreq )
		{
			break;
		}

		reply = TRUE;

		switch( ioreq->req_Public.io_Error )
		{
			case USB2Err_NoError:
			{
				hd->ErrorCnt = 0;

				data = ioreq->req_Public.io_Data;

				rn = Header_Head( & hd->Reports );

				if (( rn ) && ( rn->ReportID > 0 ))
				{
					// Uses ID's
					reportid = data[0];

					while( rn )
					{
						if ( rn->ReportID == reportid )
						{
							break;
						}
						else
						{
							rn = Node_Next( rn );
						}
					}
				}

				if (( rn ) && ( rn->Running ))
				{
					MSGPORT_PUTMSG( & rn->IOReq_MsgPort, ioreq );
					reply = FALSE;
				}
				break;
			}

			case USB2Err_Device_Detached:
			{
				USBDEBUG( "__Interrupt : Int : Detached" );
				hd->Running = FALSE;
				break;
			}

			case USB2Err_Host_Stall:
			{
				if ( ! ENDPOINTRES_DESTALL( hd->Res_Interrupt ))
				{
					hd->Running = FALSE;
					reply = FALSE;
				}
				break;
			}

			default:
			{
				hd->ErrorCnt++;

				USBDEBUG( "__Interrupt : Unknown error %ld (ErrorCnt %ld)", ioreq->req_Public.io_Error, hd->ErrorCnt );

				if ( hd->ErrorCnt >= 20 )
				{
					USBDEBUG( "__Interrupt : Too many errors, quitting" );
					hd->Running = FALSE;
					reply = FALSE;
				}
				break;
			}
		}

		if ( reply )
		{
//			USBDEBUG( "__Interrupt : Reply IORequest %p", ioreq );
			IO_SEND( ioreq );
		}
	}

	TASK_NAME_LEAVE();
}

// --

SEC_CODE void HID_Main_Report( struct USBBase *usbbase, struct HIDData *hd )
{
struct HID_ReportNode *rn;
struct ExecIFace *IExec;
U32 mask;
U32 wait;

	USBDEBUG( "HID_Report : Entry" );
	TASK_NAME_ENTER( "HID_Report" );

	IExec = usbbase->usb_IExec;

	// --

	wait = SIGBREAKF_CTRL_C;
	wait |=	hd->Res_Interrupt->MsgPortBit;
	wait |=	hd->Register->reg_Public.Stack_MsgPortBit;

	hd->Running = TRUE;

	while( TRUE )
	{
		if ( hd->Running )
		{
			mask = TASK_WAIT( wait );
		}
		else
		{
			if ( hd->ASync_Drivers.ua_Counter <= 0 )
			{
				break;
			}
			else
			{
				// -- Signal Subtask(s)
				IExec->Disable();

				rn = Header_Head( & hd->Reports );

				while( rn )
				{
					if ( rn->TaskAdr )
					{
						rn->Running = FALSE;
						TASK_SIGNAL( rn->TaskAdr, SIGBREAKF_CTRL_D );
					}

					rn = Node_Next( rn );
				}

				IExec->Enable();
				// --

				// Wait 1 sec
				IO_DO( & hd->Timer_IOReq );

				// Get Current Signals
				mask = TASK_SETSIGNAL( 0, 0 );
			}
		}

		if ( mask &	hd->Res_Interrupt->MsgPortBit )
		{
			__Interrupt( usbbase, hd );
		}

		if ( mask &	hd->Register->reg_Public.Stack_MsgPortBit )
		{
			USBDEBUG( "HID : Got Stack Bit" );

			__Stack( usbbase, hd );
		}

		if ( mask & SIGBREAKF_CTRL_C )
		{
			USBDEBUG( "HID_Report : Got CTRL+C : Exit Signal" );

			hd->Running = FALSE;
		}
	}

	// --

	TASK_NAME_LEAVE();
}

// --
