
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "Master.h"

// --

SEC_CODE static void
__myProcess_ScanDriverDir( struct USBBase *usbbase )
{
	struct ExamineData *dat;
	struct DOSIFace *IDOS;

	PTR context;
	S32 oldlock;
	S32 oldwindow;
	char pattern[64];

	IDOS = usbbase->usb_IDOS;

	oldwindow = MISC_SETPROCWINDOW( -1 );

	usbbase->usb_DriverDirLock = IDOS->Lock( DRIVERDIR, ACCESS_READ );

	if ( ! usbbase->usb_DriverDirLock )
	{
		usbbase->usb_IExec->DebugPrintF( "USB2: Could not open driver directory %s\n", DRIVERDIR );

		MISC_SETPROCWINDOW( oldwindow );

		return;
	}

	oldlock = MISC_SETCURRENTDIR( usbbase->usb_DriverDirLock );

	if ( IDOS->ParsePatternNoCase( "#?.class", pattern, sizeof( pattern ) ) != -1 )
	{
		context = IDOS->ObtainDirContextTags( EX_LockInput, usbbase->usb_DriverDirLock,
			EX_MatchString, pattern, EX_DataFields, EXF_TYPE | EXF_NAME, TAG_END );

		if ( context )
		{
			while ( ( dat = IDOS->ExamineDir( context ) ) )
			{
				if ( EXD_IS_FILE( dat ) )
				{
					FDRIVER_LOAD( dat->Name );
				}
			}

			IDOS->ReleaseDirContext( context );
		}
		else
		{
			usbbase->usb_IExec->DebugPrintF( "USB2: Could not scan driver directory %s\n", DRIVERDIR );
		}
	}

	MISC_SETCURRENTDIR( oldlock );

	MISC_SETPROCWINDOW( oldwindow );
}

// --
