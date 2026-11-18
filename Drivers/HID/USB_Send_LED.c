
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

/*
** This function is called from subtasks
** but it uses the Main HID driver resources
*/

SEC_CODE void
USB_Send_LED( struct USBBase *usbbase, struct HID_ReportNode *rn, U32 ReportID, U32 LEDStat )
{
struct USB2_IORequest *ioreq;
struct USB2_SetupData *sd;
struct HIDData *hd;
struct MsgPort *reply;

U16 ifcnr;
U16 length;
U8 report[2];
U8 id;

	hd = rn->HIDData;
	id = ReportID & 0xff;
	length = ( id ) ? 2 : 1;
	report[0] = id;
	report[length - 1] = LEDStat & 0xff;
	ifcnr = hd->StartMsg->InterfaceDescriptor->InterfaceNumber;

	TASK_NAME_ENTER( "USB_Send_LED" );

	SEMAPHORE_OBTAIN( & hd->Semaphore );

	// --

	sd = hd->Res_Control->SetupData;
	sd->RequestType			= REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface;
	sd->RequestCode			= REQCODE_Set_Report;
	sd->Value				= LE_SWAP16( 0x0200 | id );	// Output Report | ID
	sd->Index				= LE_SWAP16( ifcnr );
	sd->Length				= LE_SWAP16( length );

	ioreq = hd->Res_Control->IORequests[0];
	reply = ioreq->io_Message.mn_ReplyPort;
	// The synchronous transfer must signal the keyboard subtask.
	ioreq->io_Message.mn_ReplyPort = & rn->Input_MsgPort.ump_MsgPort;
	ioreq->io_Command		= CMD_WRITE;
	ioreq->io_Data			= report;
	ioreq->io_Length		= length;
	ioreq->io_SetupData		= sd;
	ioreq->io_SetupLength	= sizeof( struct USB2_SetupData );
	ioreq->io_TimeOut		= 1000000;

	IO_DO( ioreq );

	ioreq->io_Message.mn_ReplyPort = reply;
	ioreq->io_Data = NULL;
	ioreq->io_Length = 0;

	if ( ioreq->io_Error )
	{
		USBDEBUG( "HID : Set LED report %lu failed (%ld)", ReportID, (S32) ioreq->io_Error );
	}

	// --

	SEMAPHORE_RELEASE( & hd->Semaphore );

	TASK_NAME_LEAVE();
}
