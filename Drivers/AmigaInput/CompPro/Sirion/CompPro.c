
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
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


