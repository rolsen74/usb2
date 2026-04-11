
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

SEC_CODE S32 Report_Parse_Main( struct USBBase *usbbase, struct HIDData *hd, struct pintern *pi )
{
struct HID_ReportNode *rn;
S32 error;
HID_DEBUG( char spcbuf[64]; )

	rn = pi->rn;

	error = TRUE;

	switch( pi->bTag )
	{
		case 8:  // Input
		case 9:  // Output
		case 11: // Feature
		{

HID_DEBUG( 
{
U32 uu;

	for( uu=0 ; uu<pi->level*2 ; uu++ )
	{
		spcbuf[uu] = '.';
	}
	spcbuf[uu++] = ':';
	spcbuf[uu++] = ' ';
	spcbuf[uu++] = ' ';
	spcbuf[uu++] = ' ';
	spcbuf[uu] = 0;
})

			/**/ if ( pi->bTag == 8 ) // Input
			{
				// kind = 8 input
				// flags = dval

				HID_DEBUG( 
					usbbase->usb_IExec->DebugPrintF( "Main - Input\n" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x001 ) ? "Constant" : "Data" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x002 ) ? "Variable" : "Array" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x004 ) ? "Relative" : "Absolute" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x008 ) ? "Wrap" : "No Wrap" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x010 ) ? "Non Linear" : "Linear" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x020 ) ? "No Preferred" : "Preferred State" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x040 ) ? "Null State" : "No Null State" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x100 ) ? "Buffered Bytes" : "Bit Field" );
				)

				if (( rn->ReportID ) && ( ! rn->Offset_In ))
				{
					rn->Offset_In += 8;
				}

				// --

				pi->bn = Node_Bit_Alloc( usbbase );

				if ( ! pi->bn )
				{
					goto bailout;
				}

				pi->bn->BitOffset	= rn->Offset_In;
				pi->bn->BitCount	= pi->gn->ReportCount;
				pi->bn->BitSize		= pi->gn->ReportSize;
				pi->bn->ReportID	= pi->gn->ReportID;
				pi->bn->Flags		= pi->uval;

				while( TRUE )
				{
					pi->un = NODE_REMHEAD( & rn->UsageHeader );

					if ( ! pi->un )
					{
						break;
					}

					NODE_ADDTAIL( & pi->bn->UsageHeader, pi->un );
				}

				NODE_ADDTAIL( & rn->BitHeader, pi->bn );

				// --

				rn->Offset_Temp	+= pi->gn->ReportCount * pi->gn->ReportSize ;
				rn->Offset_In	+= rn->Offset_Temp;
			}
			else if ( pi->bTag == 9 ) // Output
			{
				// kind = 9 output
				// flags = dval

				HID_DEBUG( 
					usbbase->usb_IExec->DebugPrintF( "Main - Output\n" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x001 ) ? "Constant" : "Data" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x002 ) ? "Variable" : "Array" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x004 ) ? "Relative" : "Absolute" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x008 ) ? "Wrap" : "No Wrap" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x010 ) ? "Non Linear" : "Linear" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x020 ) ? "No Preferred" : "Preferred State" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x040 ) ? "Null State" : "No Null State" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x080 ) ? "Volatile" : "Non Volatile" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x100 ) ? "Buffered Bytes" : "Bit Field" );
				)

				if (( rn->ReportID ) && ( ! rn->Offset_Out ))
				{
					rn->Offset_Out += 8;
				}

				rn->Offset_Temp += pi->gn->ReportCount * pi->gn->ReportSize ;
				rn->Offset_Out  += rn->Offset_Temp;
			}
			else if ( pi->bTag == 11 ) // Feature
			{
				HID_DEBUG(
					usbbase->usb_IExec->DebugPrintF( "Main - Feature\n" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x001 ) ? "Constant" : "Data" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x002 ) ? "Variable" : "Array" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x004 ) ? "Relative" : "Absolute" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x008 ) ? "Wrap" : "No Wrap" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x010 ) ? "Non Linear" : "Linear" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x020 ) ? "No Preferred" : "Preferred State" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x040 ) ? "Null State" : "No Null State" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x080 ) ? "Volatile" : "Non Volatile" );
					usbbase->usb_IExec->DebugPrintF( "%s - %s\n", spcbuf, ( pi->uval & 0x100 ) ? "Buffered Bytes" : "Bit Field" );
				)

				if (( rn->ReportID ) && ( ! rn->Offset_Feat ))
				{
					rn->Offset_Feat += 8;
				}

				rn->Offset_Temp += pi->gn->ReportCount * pi->gn->ReportSize ;
				rn->Offset_Feat += rn->Offset_Temp;
			}

			rn->Offset_Temp = 0;

			while( TRUE )
			{
				pi->un = NODE_REMHEAD( & rn->UsageHeader );

				if ( ! pi->un )
				{
					break;
				}

				Node_Usage_Free( usbbase, pi->un );
			}
			break;
		}

		case 10: // Start Collection
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Main - Start Collection - " ); )

			switch( pi->uval & pi->mask )
			{
				case 0x00: // Physical
				{
					HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Physical\n" ); )
					break;
				}

				case 0x01: // Application
				{
					HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Application\n" ); )
					break;
				}

				case 0x02: // Logical
				{
					HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Logical\n" ); )
					break;
				}

				case 0x03: // Report
				{
					HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Report\n" ); )
					break;
				}

				case 0x04: // Named Array
				{
					HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Named Array\n" ); )
					break;
				}

				case 0x05: // Usage Switch
				{
					HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Usage Switch\n" ); )
					break;
				}

				case 0x06: // Usage Modifier
				{
					HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Usage Modifier\n" ); )
					break;
				}

				default:
				{
					HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "\n" ); )
					break;
				}
			}

			// --

			if ( pi->level == 0 )
			{
				if ( pi->rn )
				{
//			usbbase->usb_IExec->DebugPrintF( "hmmm uuu11 not null\n" );
					goto bailout;
				}

				pi->rn = rn = Node_Report_Alloc( usbbase, hd );

				if ( ! pi->rn )
				{
//			usbbase->usb_IExec->DebugPrintF( "Report_Parse() out of mem\n" );
					goto bailout;
				}

				NODE_ADDTAIL( & pi->reports, pi->rn );

//				rn->ReportID = pi->gn->ReportID;

				// --

				while( TRUE )
				{
					PTR n = NODE_REMHEAD( & pi->usages );

					if ( ! n )
					{
						break;
					}

					NODE_ADDTAIL( & rn->UsageHeader, n );
				}
			}

			pi->level++;

			// --

			pi->un = NODE_REMTAIL( & rn->UsageHeader );

			if ( ! pi->un )
			{
//				usbbase->usb_IExec->DebugPrintF( "NULL Pointer\n" );
				goto bailout;
			}

			if ( rn->UsageID == 0 )
			{
				 rn->UsageID = pi->un->Min;
			}

			Node_Usage_Free( usbbase, pi->un );

			break;
		}

		case 12: // End Collection
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Main - End Collection\n" ); )

//--		if ( cn == NULL )
//--		{
//--			usbbase->usb_IExec->DebugPrintF( "NULL Pointer\n" );
//--			goto bailout;
//--		}
//--
//--		NODE_ADDTAIL( rn->Collections, NULL, cn );
//--
//--		cn->Report = pi->rn;

			// ---

			if ( ! pi->level )
			{
//	usbbase->usb_IExec->DebugPrintF( "ooooo 123\n" );
				goto bailout;
			}

			pi->level--;

			if ( ! pi->level )
			{
				pi->rn = rn = NULL;
			}

			// ---

//--		cn = cn->Parent;
			break;
		}

		default:
		{
//			usbbase->usb_IExec->DebugPrintF( "Unsupported Main Tag Type (%lu)", pi->bTag );
			break;
		}
	}

	error = FALSE;

bailout:

	return( error );
}

// --
