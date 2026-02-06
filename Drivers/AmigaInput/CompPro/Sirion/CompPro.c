
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

struct Library *			NewlibBase			= NULL;
struct Library *			USBResourceBase		= NULL;

struct ExecIFace *			IExec				= NULL;
struct Interface *			INewlib				= NULL;
struct USBResourceIFace *	IUSBResource		= NULL;
struct USBSysIFace *		IUSBSys				= NULL;

// --

S32 _start( STRPTR argstring UNUSED, S32 arglen UNUSED, PTR SysBase UNUSED )
{
	return( 0 );
}

// --


// --


