
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

SEC_CODE S32 Report_Parse_Local( struct USBBase *usbbase, struct HIDData *hd, struct pintern *pi )
{
struct HID_ReportNode *rn;
struct USB2_Header *list;
S32 error;

	error	= TRUE;

	rn		= pi->rn;
	list	= ( rn ) ? & rn->UsageHeader : & pi->usages ;

	switch( pi->bTag )
	{
		case 0: // Usage
		{
			pi->uval = (( pi->gn->UsagePage ) | ( pi->uval & pi->mask ));

			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Local - Usage (%08lx) - ", pi->uval ); )

			if ( pi->gn->UsagePage == 0x00010000 )
			{
				switch( pi->uval & pi->mask )
				{
					case 0x01: // Pointer
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Pointer\n" ); )
						break;
					}

					case 0x02: // Mouse
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Mouse\n" ); )
						break;
					}

					case 0x04: // Joystick
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Joystick\n" ); )
						break;
					}

					case 0x05: // Game Pad
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Game Pad\n" ); )
						break;
					}

					case 0x06: // Keyboard
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Keyboard\n" ); )
						break;
					}

					case 0x07: // Keypad
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Keypad\n" ); )
						break;
					}

					case 0x09: // Button
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Button\n" ); )
						break;
					}

					case 0x30: // X
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - X\n" ); )
						break;
					}

					case 0x31: // Y
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Y\n" ); )
						break;
					}

					case 0x38: // Wheel
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Wheel\n" ); )
						break;
					}

					case 0x0080: // System Control
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - System Control\n" ); )
						break;
					}

					case 0x0081: // System Power Down
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - System Power Down\n" ); )
						break;
					}

					case 0x0082: // System Sleep
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - System Sleep\n" ); )
						break;
					}

					case 0x0083: // System Wake Up
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - System Wake Up\n" ); )
						break;
					}

					default:
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "\n" ); )
						break;
					}
				}
			}
			else if ( pi->gn->UsagePage == 0x00080000 )
			{
				switch( pi->uval & pi->mask )
				{
					case 0x003b: // Usage In Use Indicator
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Usage in use indicator\n" ); )
						break;
					}

					default:
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "\n" ); )
						break;
					}
				}
			}
			else if ( pi->gn->UsagePage == 0x000c0000 )
			{
				switch( pi->uval & pi->mask )
				{
					case 0x0001: // Consumer Control
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Consumer Control\n" ); )
						break;
					}

					case 0x0002: // Numberic Key Pad
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Numberic Key Pad\n" ); )
						break;
					}

					case 0x00b5: // Scan Next Track
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Scan Next Track\n" ); )
						break;
					}

					case 0x00b6: // Scan Previous Track
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Scan Previous Track\n" ); )
						break;
					}

					case 0x00b7: // Stop
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Stop\n" ); )
						break;
					}

					case 0x00cd: // Play/Pause
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Play/Pause\n" ); )
						break;
					}

					case 0x00e2: // Mute
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Mute\n" ); )
						break;
					}

					case 0x00e9: // Volume Up
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Volume Up\n" ); )
						break;
					}

					case 0x00ea: // Volume Down
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Volume Down\n" ); )
						break;
					}

					case 0x0183: // AC Comsumer Control Config
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - Consumer Control Config\n" ); )
						break;
					}

					case 0x018a: // AL Email Reader
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - AL Email Reader\n" ); )
						break;
					}

					case 0x0192: // AL Calculator
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - AL Calculator\n" ); )
						break;
					}

					case 0x0221: // AC Search
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - AC Search\n" ); )
						break;
					}

					case 0x0223: // AC Home
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - AC Home\n" ); )
						break;
					}

					case 0x0224: // AC Back
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - AC Back\n" ); )
						break;
					}

					case 0x0225: // AC Forward
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - AC Forward\n" ); )
						break;
					}

					case 0x022a: // AC Bookmarks
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Page - AC Bookmarks\n" ); )
						break;
					}

					default:
					{
						HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "\n" ); )
						break;
					}
				}
			}
			else
			{
				HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "\n" ); )
			}

			if ( Report_Add_Usage( usbbase, hd, list, pi->uval, pi->uval ))
			{
//				usbbase->usb_IExec->DebugPrintF( "Error Adding Usage Node\n" );
				goto bailout;
			}
			break;
		}

		case 1: // Usage Minimum
		case 2: // Usage Maximum
		{
			pi->uval = (( pi->gn->UsagePage ) | ( pi->uval & pi->mask ));

			if ( pi->bTag == 1 )
			{
				pi->minusage = pi->uval;
				pi->usageflag |= 0x01;

				HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Local - Usage Minimum (%08lx)\n", pi->uval ); )
			}
			else
			{
				pi->maxusage = pi->uval;
				pi->usageflag |= 0x02;

				HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Local - Usage Maximum (%08lx)\n", pi->uval ); )
			}

			if ( pi->usageflag == 3 )
			{
				if ( Report_Add_Usage( usbbase, hd, list, pi->minusage, pi->maxusage ))
				{
//					usbbase->usb_IExec->DebugPrintF( "Error Adding Usage Node\n" );
					goto bailout;
				}

				pi->usageflag = 0;
			}
			break;
		}

		case 3: // Designator Index
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Local - Designator Index\n" ); )

//--		if ( HID_AddUsage( devBase, rn, &r->Designators, pi->uval, pi->uval ))
//--		{
//--			usbbase->usb_IExec->DebugPrintF( "Error Adding Usage Node\n" );
//--			goto bailout;
//--		}
			break;
		}

		case 4: // Designator Minimum
		case 5: // Designator Maximum
		{
			if ( pi->bTag == 4 )
			{
//				pi->mindesign = pi->uval;
				pi->designflag |= 0x01;

				HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Local - Designator Minimum (%08lx)\n", pi->uval ); )
			}
			else
			{
//				pi->maxdesign = pi->uval;
				pi->designflag |= 0x02;

				HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Local - Designator Maximum (%08lx)\n", pi->uval ); )
			}
						
//--		if ( pi->designflag == 0x03 )
//--		{
//--			if ( HID_AddUsage( devBase, rn, &r->Designators, pi->mindesign, pi->maxdesign ))
//--			{
//--				usbbase->usb_IExec->DebugPrintF( "Error Adding Usage Node\n" );
//--				goto bailout;
//--			}
//--
//--			pi->designflag = 0;
//--		}
			break;
		}

		// 6 is not definded

		case 7: // String Index
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Local - String Index\n" ); )

//--		if ( HID_AddUsage( devBase, rn, &r->Strings, pi->uval, pi->uval ))
//--		{
//--			usbbase->usb_IExec->DebugPrintF( "Error Adding Usage Node\n" );
//--			goto bailout;
//--		}
			break;
		}

		case 8: // String Minimum
		case 9: // String Maximum
		{
			if ( pi->bTag == 8 )
			{
//				pi->minstring = pi->uval;
				pi->stringflag |= 0x01;

				HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Local - String Minimum (%08lx)\n", pi->uval ); )
			}
			else
			{
//				pi->maxstring = pi->uval;
				pi->stringflag |= 0x02;

				HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Local - String Maximum (%08lx)\n", pi->uval ); )
			}

//--		if ( pi->stringflag == 0x03 )
//--		{
//--			if ( HID_AddUsage( devBase, rn, &r->Strings, pi->minstring, pi->maxstring ))
//--			{
//--				usbbase->usb_IExec->DebugPrintF( "Error Adding Usage Node\n" );
//--				goto bailout;
//--			}
//--
//--			pi->stringflag = 0;
//--		}
			break;
		}

//		case 10: // Delimiter

		default:
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Unsupported Local Tag Type (%lu)\n", pi->bTag ); )
			break;
		}
	}

	error = FALSE;

bailout:

	return( error );
}

// --
