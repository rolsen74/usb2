
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

SEC_CODE S32 Report_Parse_Global( struct USBBase *usbbase, struct HIDData *hd, struct pintern *pi )
{
struct HID_GlobalNode *new;
struct HID_GlobalNode *cur;
struct HID_ReportNode *rn;
S32 error;

	rn = pi->rn;

	cur = pi->gn;

	error = TRUE;

	switch( pi->bTag )
	{
		case 0: // Usage Page
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Global - Usage Page (%04lx) - ", pi->uval ); )

			switch( pi->uval )
			{
				case 0x01: // Generic Desktop
				{
					HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Generic Desktop\n" ); )
					break;
				}

				case 0x07: // Keyboard
				{
					HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Keyboard\n" ); )
					break;
				}

				case 0x09: // Button
				{
					HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Button\n" ); )
					break;
				}

				case 0x0c: // Consumer
				{
					HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Consumer\n" ); )
					break;
				}

				default:
				{
					HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "\n" ); )
					break;
				}
			}

			cur->UsagePage = ( pi->uval << 16 );
			break;
		}

		case 1: // Logical Minimum
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Global - Logical Minimum (%ld)\n", pi->sval ); )

			cur->MinLogical = pi->sval;
			break;
		}

		case 2: // Logical Maximum
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Global - Logical Maximum (%ld)\n", pi->sval ); )

			cur->MaxLogical = pi->sval;
			break;
		}

		case 3: // Physical Minimum
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Global - Physical Minimum (%ld)\n", pi->sval ); )

			cur->MinPhysical = pi->sval;
			break;
		}

		case 4: // Physical Maximum
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Global - Physical Maximum (%ld)\n", pi->sval ); )

			cur->MaxPhysical = pi->sval;
			break;
		}

		case 5: // Unit Exponent
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Global - Unit Exponent (%lu)\n", pi->uval ); )

			cur->UnitExponent = pi->uval;
			break;
		}

		case 6: // Unit
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Global - Unit (%lu)\n", pi->uval ); )

			cur->Unit = pi->uval;
			break;
		}

		case 7: // Report Size
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Global - Report Size (%lu)\n", pi->uval ); )

			cur->ReportSize = pi->uval;
			break;
		}

		case 8: // Report ID
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Global - Report ID (%lu)\n", pi->uval ); )

			cur->ReportID = pi->uval;

			if ( rn )
			{
				/**/ if ( rn->ReportID == 0 )
				{
					rn->ReportID = pi->uval;
				}
				else if ( rn->ReportID != pi->uval )
				{
					HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "ReportID already set : Old %lu, New %lu\n", rn->ReportID, pi->uval ); )
				}
			}
			break;
		}

		case 9: // Report Count
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Global - Report Count (%lu)\n", pi->uval ); )

			cur->ReportCount = pi->uval;
			break;
		}

		case 10: // Push
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Global - Push\n" ); )

			new = Node_Global_Alloc( usbbase );

			if ( ! new )
			{
//				usbbase->usb_IExec->DebugPrintF( "Error allocating memory\n" );
				goto bailout;
			}

			MEM_COPY( pi->gn, new, sizeof( struct HID_GlobalNode ));

			new->Parent = pi->gn;

			pi->gn = cur = new;
			break;
		}

		case 11: // Pop
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Global - Pop\n" ); )

			new = cur->Parent;

			if ( ! new )
			{
//				usbbase->usb_IExec->DebugPrintF( "NULL Pointer\n" );
				goto bailout;
			}

			Node_Global_Free( usbbase, pi->gn );

			pi->gn = cur = new;
			break;
		}

		default:
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Unsupported Global Tag Type (%lu)\n", pi->bTag ); )
			break;
		}
	}

	error = FALSE;

bailout:

	return( error );
}

// --
