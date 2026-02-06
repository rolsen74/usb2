
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

S32 Driver_Report_Get( struct USB_Driver *in )
{
struct USBBusSetupData sd;
struct HID_DescItem *item;
struct USBIOReq *ioreq;
struct HID_Desc *dsc;
S32 retval;
S32 stat;
U32 cnt;


	retval = FALSE;

	// --- Find Report Descriptor
	// Add Support for Physical Descriptor? 0x23 ?
	// There can max be 83 Descriptors

	dsc = in->USB_HID_Dsc;

	for( cnt=0 ; cnt<dsc->NumDescs ; cnt++ )
	{
		item = & dsc->Descs[cnt];

		#ifdef DEBUG
		DebugPrintF( "%2lu Type...: %lu\n", cnt, (U32) item->Type );
		DebugPrintF( "%2lu Length.: %lu\n", cnt, (U32) LE_SWAP16( item->Length ));
		#endif

		if ( item->Type == 0x22 )	// 0x22 HID Report, 0x23 Physical Report
		{
			break;
		}
	}

	if ( cnt == dsc->NumDescs )
	{
		#ifdef DEBUG
		DebugPrintF( "Unable to find HID Report (0x22)\n" );
		#endif
		goto bailout;
	}

	// --- Get Report Desciptor

	item = & dsc->Descs[cnt];

	in->Report_Size		= LE_SWAP16( item->Length );
	in->Report_Buffer	= AllocVecTags( in->Report_Size,
		AVT_Clear, 0,
		TAG_END
	);

	if ( ! in->Report_Buffer )
	{
		#ifdef DEBUG
		DebugPrintF( "Error allocating memory (Size %lu)\n", in->Report_Size );
		#endif
		goto bailout;
	}

	// Get Report
	sd.sd_RequestType = USBSDT_DIR_DEVTOHOST | USBSDT_TYP_STANDARD | USBSDT_REC_INTERFACE;
	sd.sd_Request	= 0x06; // Get Desc
	sd.sd_Value		= LE_SWAP16( item->Type << 8 | 0 );
	sd.sd_Index		= LE_SWAP16( in->USB_Interface_Nr );
	sd.sd_Length	= LE_SWAP16( in->Report_Size );

	ioreq = in->USB_Ctrl_IOReq;
	ioreq->io_SetupLength = sizeof( struct USBBusSetupData );
	ioreq->io_SetupData = & sd;
	ioreq->io_EndPoint = in->USB_Ctrl_EP_Ref;
	ioreq->io_Command = CMD_READ;
	ioreq->io_Length = in->Report_Size;
	ioreq->io_Data = in->Report_Buffer;

	stat = DoIO( (PTR) ioreq );

	if ( stat )
	{
		#ifdef DEBUG
		DebugPrintF( "Error Getting HID Report (%ld)\n", stat );
		#endif
		goto bailout;
	}

	retval = TRUE;

bailout:

	return( retval );
}
