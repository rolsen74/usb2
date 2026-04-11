
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

SEC_CODE void USB_Send_LED( struct USBBase *usbbase, struct HIDData *hd, U32 ReportID, U32 LEDStat )
{
struct USB2_IORequest *ioreq;
struct USB2_SetupData *sd;
U16 ifcnr;
U8 leds;
U8 id;

	// --

	TASK_NAME_ENTER( "USB_Send_LED" );

	SEMAPHORE_OBTAIN( & hd->Semaphore );

	// --

	id		= ReportID & 0xff;
	leds	= LEDStat  & 0xff;
	ifcnr	= hd->StartMsg->InterfaceDescriptor->InterfaceNumber;

	sd = hd->Res_Control->SetupData;
	sd->RequestType			= REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface;
	sd->RequestCode			= REQCODE_Set_Report;
	sd->Value				= LE_SWAP16( (( 0x0200 ) + ( id )) );	// Output Report | ID
	sd->Index				= LE_SWAP16( ifcnr );
	sd->Length				= LE_SWAP16( 1 );

	ioreq = hd->Res_Control->IORequests[0];
	ioreq->io_Command		= CMD_WRITE;
	ioreq->io_Data			= & leds;
	ioreq->io_Length		= 1;
	ioreq->io_SetupData		= sd;
	ioreq->io_SetupLength	= sizeof( struct USB2_SetupData );
	ioreq->io_TimeOut		= 1000000;

usbbase->usb_IExec->DebugPrintF( "Send LED : sd->Value : $%04lx : ID %ld, LEDs $%04lx\n", (U32) LE_SWAP16( sd->Value ), ReportID, LEDStat );

	// hmm looks like we have a EHCI Control Write problem... 
	// need to look at that this sometime
//	IO_DO( ioreq );

usbbase->usb_IExec->DebugPrintF( "Send LED : io_Error : %ld\n", (S32) ioreq->io_Error );

	// --

	SEMAPHORE_RELEASE( & hd->Semaphore );

	TASK_NAME_LEAVE();
}
