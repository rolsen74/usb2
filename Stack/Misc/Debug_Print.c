
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "Debug_Print.h"

// --

#if defined( DO_DEBUG ) || defined( DO_ERROR ) || defined( DO_INFO )

SEC_CODE void
USB2_DebugPrint( struct USBBase *usbbase, STR fmt, PTR args )
{
	struct UtilityIFace *IUtility;
	struct ExecIFace *IExec;
	struct Task *self;

	STR buf;

	IUtility = usbbase->usb_IUtility;
	buf = NULL;

	IExec = USB2_GetExecIFace();

	if ( IUtility )
	{
		buf = IUtility->VASPrintf( fmt, args );
	}

	IExec->Disable();

	self = IExec->FindTask( NULL );

	IExec->DebugPrintF( "%s%p: %s\n", ( self != usbbase->_dbg_Task ) ? "--\n" : "",
		self, ( buf ) ? buf : fmt );

	usbbase->_dbg_Task = self;

	IExec->Enable();

	if ( buf )
	{
		IExec->FreeVec( buf );
	}
}

#endif

// --
