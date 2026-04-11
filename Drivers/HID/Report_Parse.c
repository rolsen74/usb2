
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

SEC_CODE S32 Report_Parse( struct USBBase *usbbase, struct HIDData *hd )
{
struct HID_ReportNode *rn;
struct HID_GlobalNode gstack;
struct pintern pi;
U32 max_bits;
S32 error;
U8 prefix;

HID_DEBUG(
usbbase->usb_IExec->Disable();
usbbase->usb_IExec->DebugPrintF( "\n\n## ---- ##\n" ); )

	// --

	MEM_SET( & gstack,	0, sizeof( struct HID_GlobalNode ) );
	MEM_SET( & pi,		0, sizeof( struct pintern ) );

	pi.data		= hd->ReportBuffer;
	pi.size		= hd->ReportSize;
	pi.end		= & pi.data[pi.size];
	pi.gn		= & gstack;

	error		= TRUE;

	// --

	if (( ! pi.data ) || ( ! pi.size ))
	{
		goto bailout;
	}

	// --

	while(( pi.data ) && ( pi.data < pi.end ))
	{
		pi.bSize	= ( *pi.data & 0x03 ) >> 0;
		pi.bType	= ( *pi.data & 0x0c ) >> 2;
		pi.bTag		= ( *pi.data & 0xf0 ) >> 4;

HID_DEBUG(
{
	for( U32 uu = 0 ; uu < pi.level ; uu++ )
	{
		usbbase->usb_IExec->DebugPrintF( ".." );
	}

	usbbase->usb_IExec->DebugPrintF( ": %02lx : ", (U32) pi.data[0] );
})

		prefix = *pi.data++;

		if ( prefix == 0xfe )
		{
			// Skip Long Tag Item

			if ( pi.data + 2 > pi.end )
			{
				// Out of bound
// usbbase->usb_IExec->DebugPrintF( "LongTag: Out of bound 1\n" );
				goto bailout;
			}

			pi.len  = *pi.data++;
			pi.bTag = *pi.data++;

			if ( pi.data + pi.len > pi.end )
			{
				// Out of bound
// usbbase->usb_IExec->DebugPrintF( "LongTag: Out of bound 2 : Len %lu\n", (U32) pi.len );
				goto bailout;
			}

			pi.data += pi.len;

//			usbbase->usb_IExec->DebugPrintF( "Skipping Long Tag Item\n" );
			continue;
		}

		if ( Report_Read_Item( usbbase, hd, & pi ))
		{
// usbbase->usb_IExec->DebugPrintF( "Report_Read_Item() failed\n" );
			goto bailout;
		}

		switch( pi.bType )
		{
			case 0:	// Main
			{
				if ( Report_Parse_Main( usbbase, hd, & pi ))
				{
// usbbase->usb_IExec->DebugPrintF( "Report_Parse_Main() failed\n" );
					goto bailout;
				}
				break;
			}

			case 1:	// Global
			{
				if ( Report_Parse_Global( usbbase, hd, & pi ))
				{
// usbbase->usb_IExec->DebugPrintF( "Report_Parse_Global() failed\n" );
					goto bailout;
				}
				break;
			}

			case 2:	// Local
			{
				if ( Report_Parse_Local( usbbase, hd, & pi ))
				{
// usbbase->usb_IExec->DebugPrintF( "Report_Parse_Local() failed\n" );
					goto bailout;
				}
				break;
			}

			default:
			{
//				usbbase->usb_IExec->DebugPrintF( "Unknown Item Type\n" );
				goto bailout;
			}
		}

//usbbase->usb_IExec->DebugPrintF( "\n" );

	}

	// --

	max_bits = 0;

	rn = pi.reports.uh_Head;

	while( rn )
	{
// usbbase->usb_IExec->DebugPrintF( "ReportID %2ld, In Offset %2ld, Out Offset %2ld, Feat Offset %2ld\n", rn->ReportID, rn->Offset_In, rn->Offset_Out, rn->Offset_Feat );

		if ( rn->Offset_In > max_bits )
		{
			max_bits = rn->Offset_In;
		}

		rn = Node_Next( rn );
	}

	hd->InputBufferSize = ( max_bits + 7 ) / 8 ;

// usbbase->usb_IExec->DebugPrintF( "Input Buffer : Bits %ld, Bytes %ld\n", max_bits, hd->InputBufferSize );

	// --

	if ( hd->InputBufferSize == 0 )
	{
// usbbase->usb_IExec->DebugPrintF( "Report_Parse() zero buffer size\n" );
		goto bailout;
	}

	// --
	error = FALSE;

bailout:

	// --
	// Free lets over reports

	while( TRUE )
	{
		rn = NODE_REMHEAD( & pi.reports );

		if ( ! rn )
		{
			break;
		}

		if ( error )
		{
			Node_Report_Free( usbbase, rn );
		}
		else
		{
			// Move reports and to public
			NODE_ADDTAIL( & hd->Reports, rn );
		}
	}

	// --
	// Make sure we don't leave with stack'ed Globals

	pi.gn = gstack.Parent;

	while( pi.gn )
	{
		PTR next = pi.gn->Parent;

		Node_Global_Free( usbbase, pi.gn );

		pi.gn = next;
	}

	// --

// usbbase->usb_IExec->DebugPrintF( "InputBufferSize %ld, Type %ld, Mode %ld\n", hd->InputBufferSize, hd->Driver_Type, hd->Driver_Mode );
// usbbase->usb_IExec->Enable();

	return(	error );
}
