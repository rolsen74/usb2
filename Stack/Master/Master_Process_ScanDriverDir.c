
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

SEC_CODE static S32
__myProcess_ScanDriverDir ( struct USBBase * usbbase )
{
	struct ExamineData * dat;
	struct DOSIFace *	 IDOS;

	PTR	 context;
	S32	 oldlock;
	S32	 oldwindow;
	S32	 error;
	STR	 operation;
	char pattern[64];
	BPTR lock;

	IDOS = usbbase->usb_IDOS;

	oldwindow = MISC_SETPROCWINDOW ( -1 );

	operation = "lock";

	lock = IDOS->Lock ( DRIVERDIR, ACCESS_READ );

	if ( ! lock )
	{
		error = IDOS->IoErr ();

		goto bailout;
	}

	oldlock = MISC_SETCURRENTDIR ( lock );

	operation = "parse match pattern";

	if ( IDOS->ParsePatternNoCase ( "#?.class", pattern, sizeof ( pattern ) ) == -1 )
	{
		error = IDOS->IoErr ();

		goto restore;
	}

	operation = "open enumeration";

	context = IDOS->ObtainDirContextTags ( EX_LockInput, lock, EX_MatchString, pattern, EX_DataFields, EXF_TYPE | EXF_NAME,
										   TAG_END );

	if ( ! context )
	{
		error = IDOS->IoErr ();

		goto restore;
	}

	operation = "enumerate";

	while ( ( dat = IDOS->ExamineDir ( context ) ) )
	{
		if ( EXD_IS_FILE ( dat ) )
		{
			FDRIVER_LOAD ( dat->Name );
		}
	}

	error = IDOS->IoErr ();

	IDOS->ReleaseDirContext ( context );

	if ( error == ERROR_NO_MORE_ENTRIES )
	{
		usbbase->usb_DriverDirLock = lock;
		operation				   = NULL;
	}

restore:

	MISC_SETCURRENTDIR ( oldlock );

	if ( operation )
	{
		IDOS->UnLock ( lock );
	}

bailout:

	MISC_SETPROCWINDOW ( oldwindow );

	if ( operation )
	{
		usbbase->usb_IExec->DebugPrintF ( "USB : Could not %s driver directory %s (DOS error %ld)\n", operation, DRIVERDIR,
										  error );
	}

	return ( ! operation );
}

// --
