
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

SEC_CODE S32 Report_Read_Report( struct USBBase *usbbase, struct HIDData *hd UNUSED, U32 ifcnr )
{
struct USB2_IORequest *ioreq;
struct USB2_SetupData *sd;
struct USB2_HID_Desc *hiddsc;
U8 data[256];
S32 error;
U32 cnt;
S32 max;

	error	= FALSE;
	ioreq	= hd->Res_Control->IORequests[0];
	sd		= hd->Res_Control->SetupData;
	hiddsc	= (PTR) data;

	// --- Get HID Descriptor

	max = 3;

	while( TRUE )
	{
		if ( max <= 0 )
		{
			break;
		}

		max--;

		sd->RequestType			= REQTYPE_Read | REQTYPE_Standard | REQTYPE_Interface;
		sd->RequestCode			= REQCODE_Get_Descriptor;
		sd->Value				= LE_SWAP16( DSCTYPE_HID << 8 );
		sd->Index				= LE_SWAP16( ifcnr );
		sd->Length				= LE_SWAP16( 255 );

		ioreq->io_Command		= CMD_READ;
		ioreq->io_Data			= data;
		ioreq->io_Length		= 255;		// Max Desciptor size
		ioreq->io_SetupData		= sd;
		ioreq->io_SetupLength	= sizeof( struct USB2_SetupData );

		IO_DO( ioreq );

		if ( ! ioreq->io_Error )
		{
			// Done
			break;
		}

		// Do delay here?
	}

	if (( ioreq->io_Error ) || ( ioreq->io_Length < 6 ))
	{
// usbbase->usb_IExec->DebugPrintF( "Error retriving HID Descriptor (%ld)\n", (S32) ioreq->io_Error );
		goto bailout;
	}

	// --- Find Report Descriptor

	// Add Support for Physical Descriptor?
	// There can max be 83 Descriptors

	for( cnt=0 ; cnt<hiddsc->NumDescriptors && cnt < 83 ; cnt++ )
	{
// usbbase->usb_IExec->DebugPrintF( "%2ld Type...: %ld\n", cnt, (S32) hiddsc->Descriptors[cnt].Type );
// usbbase->usb_IExec->DebugPrintF( "%2ld Length.: %ld\n", cnt, (S32) LE_SWAP16( hiddsc->Descriptors[cnt].Length ));

		if ( hiddsc->Descriptors[cnt].Type == 0x22 )
		{
			break;
		}
	}

	if (( cnt == hiddsc->NumDescriptors ) || ( cnt == 83 ))
	{
// usbbase->usb_IExec->DebugPrintF( "Report Descriptor not found\n" );
		goto bailout;
	}

	// --- Get Report Desciptor

	hd->ReportSize		= LE_SWAP16( hiddsc->Descriptors[cnt].Length );
	hd->ReportBuffer	= MEM_ALLOCVEC( hd->ReportSize, TRUE );

	if ( ! hd->ReportBuffer )
	{
//		usbbase->usb_IExec->DebugPrintF( "Error allocating memory (Size %ld)\n", hd->ReportSize );
		goto bailout;
	}

	max = 3;

	while( TRUE )
	{
		if ( max <= 0 )
		{
			break;
		}

		max--;

		sd->RequestType			= REQTYPE_Read | REQTYPE_Standard | REQTYPE_Interface;
		sd->RequestCode			= REQCODE_Get_Descriptor;
		sd->Value				= LE_SWAP16(( DSCTYPE_HID_REPORT << 8 | 0 ));
		sd->Index				= LE_SWAP16( ifcnr );
		sd->Length				= LE_SWAP16( hd->ReportSize );

		ioreq->io_Command		= CMD_READ;
		ioreq->io_Data			= hd->ReportBuffer;
		ioreq->io_Length		= hd->ReportSize;
		ioreq->io_SetupData		= sd;
		ioreq->io_SetupLength	= sizeof( struct USB2_SetupData );

		IO_DO( ioreq );

		if (( ioreq->io_Error == 0 ) 
		&&	( ioreq->io_Length == hd->ReportSize ))
		{
			// Done
			break;
		}

		// Do delay here?
	}

	if (( ioreq->io_Error ) || ( ioreq->io_Length != hd->ReportSize ))
	{
// usbbase->usb_IExec->DebugPrintF( "Error reading Report Descriptor (%ld)\n", ioreq->io_Error );
		goto bailout;
	}

// usbbase->usb_IExec->DebugPrintF( "Got HID Report\n", ioreq->io_Error );

	error = TRUE;

bailout:

	return( error );
}

// --
