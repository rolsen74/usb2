
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

SEC_CODE static void
SendReport( struct USBBase *usbbase, struct HID_ReportNode *rn, U8 id, U8 *report, U16 length )
{
	struct USB2_IORequest *ioreq;
	struct USB2_SetupData *sd;
	struct HIDData *hd;
	struct MsgPort *reply;

	hd = rn->HIDData;

	SEMAPHORE_OBTAIN( &hd->Semaphore );

	sd = hd->Res_Control->SetupData;
	sd->RequestType = REQTYPE_Write | REQTYPE_Class | REQTYPE_Interface;
	sd->RequestCode = REQCODE_Set_Report;
	sd->Value = LE_SWAP16( ( USBHID_REPORTTYPE_OUTPUT << 8 ) | id );
	sd->Index = LE_SWAP16( hd->StartMsg->InterfaceDescriptor->InterfaceNumber );
	sd->Length = LE_SWAP16( length );

	ioreq = hd->Res_Control->IORequests[0];
	reply = ioreq->io_Message.mn_ReplyPort;
	// Synchronous control requests must signal the keyboard worker.
	ioreq->io_Message.mn_ReplyPort = &rn->Input_MsgPort.ump_MsgPort;
	ioreq->io_Command = CMD_WRITE;
	ioreq->io_Data = report;
	ioreq->io_Length = length;
	ioreq->io_SetupData = sd;
	ioreq->io_SetupLength = sizeof( *sd );
	ioreq->io_TimeOut = 1000000;

	IO_DO( ioreq );

	ioreq->io_Message.mn_ReplyPort = reply;
	ioreq->io_Data = NULL;
	ioreq->io_Length = 0;

	if ( ioreq->io_Error )
	{
		USBDEBUG( "HID: Set LED report %lu failed (%ld)", (U32) id, (S32) ioreq->io_Error );
	}

	SEMAPHORE_RELEASE( &hd->Semaphore );
}

// --

SEC_CODE void
USB_Send_LED( struct USBBase *usbbase, struct HID_ReportNode *rn, U32 leds )
{
	struct HID_ReportLayout *layout;
	struct HID_BitNode *bn;

	U8 *report;
	U8 boot;
	U32 id;
	U32 index;
	U32 offset;
	U32 usage;
	U32 length;

	if ( rn->HIDData->Driver_Mode == HID_DMode_Boot )
	{
		boot = leds;

		SendReport( usbbase, rn, 0, &boot, sizeof( boot ) );

		return;
	}

	for ( id = 0; id < 256; id++ )
	{
		layout = rn->HIDData->Layouts[id];

		if ( ( ! layout ) || ( layout->Owner[HID_Report_Output] != rn ) )
		{
			continue;
		}

		length = ( layout->Bits[HID_Report_Output] + 7 ) / 8;
		report = NULL;

		for ( bn = Header_Head( &rn->OutputHeader ); bn; bn = Node_Next( bn ) )
		{
			if ( ( bn->ReportID != id ) || ( ! ( bn->Flags & 2 ) ) )
			{
				continue;
			}

			for ( index = 0; index < bn->BitCount; index++ )
			{
				usage = Report_Get_Usage( bn, index, TRUE );

				if ( ( usage < 0x00080001 ) || ( usage > 0x00080005 ) )
				{
					continue;
				}

				if ( ! report )
				{
					report = MEM_ALLOCVEC( length, TRUE );

					if ( ! report )
					{
						return;
					}

					report[0] = id;
				}

				if ( leds & ( 1U << ( usage - 0x00080001 ) ) )
				{
					offset = bn->BitOffset + index * bn->BitSize;
					report[offset / 8] |= 1U << ( offset & 7 );
				}
			}
		}

		if ( report )
		{
			SendReport( usbbase, rn, id, report, length );

			MEM_FREEVEC( report );
		}
	}
}

// --
