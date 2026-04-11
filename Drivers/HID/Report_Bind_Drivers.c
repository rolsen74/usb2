
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

SEC_CODE S32 Report_Bind_Drivers( struct USBBase *usbbase, struct HIDData *hd UNUSED )
{
struct HID_ReportNode *next;
struct HID_ReportNode *rn;
S32 error;
PTR init;
PTR main;
PTR free;
STR name;

	error = TRUE;

	// --

	rn = Header_Head( & hd->Reports );

	while( rn )
	{
		next = Node_Next( rn );
		// --

		main = NULL;
		name = NULL;

		switch( rn->UsageID )
		{
			case 0x00010002: // Mouse
			{
				name = "Mouse";
				init = __Mouse_Init;
				main = __Mouse_Main;
				free = __Mouse_Free;
				break;
			}

			case 0x00010006: // Keyboard
			{
				name = "Keyboard";
				init = __Keyboard_Init;
				main = __Keyboard_Main;
				free = __Keyboard_Free;
				break;
			}

			case 0x00010080: // System Control
			{
				break;
			}

			case 0x000c0001: // Consumer Control
			{
				break;
			}

			default:
			{
				break;
			}
		}

		// --

		rn->TaskName = name;

		if ( main )
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Bind ReportID : %ld\n", rn->ReportID ); )

			TASK_START(
				TASK_Tag_Func_Init, init,
				TASK_Tag_Func_Main, main,
				TASK_Tag_Func_Free, free,
				TASK_Tag_UserData, rn,
				TASK_Tag_Prioity, hd->TaskAdr->tc_Node.ln_Pri,
				TASK_Tag_ASync, & hd->ASync_Drivers,
				TASK_Tag_Type, TASK_Type_HID,
				TASK_Tag_Name, name,
				TAG_END
			);
		}
		else
		{
			HID_DEBUG( usbbase->usb_IExec->DebugPrintF( "Discard ReportID : %ld\n", rn->ReportID ); )

			NODE_REMNODE( & hd->Reports, rn );
			Node_Report_Free( usbbase, rn );
		}

		// --
		rn = next;
	}

	error = FALSE;

//bailout:

	return(	error );
}
