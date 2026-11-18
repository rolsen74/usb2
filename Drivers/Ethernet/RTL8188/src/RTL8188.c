
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct Library *		DOSBase			= NULL;
struct DOSIFace *		IDOS			= NULL;

struct Library *		SysBase			= NULL;
struct ExecIFace *		IExec			= NULL;

struct Library *		NewlibBase		= NULL;
struct Interface *		INewlib			= NULL;

struct Library *		UtilityBase		= NULL;
struct UtilityIFace *	IUtility		= NULL;

// --

struct MsgPort *		USB2MsgPort		= NULL;
struct IORequest *		USB2IOReq		= NULL;
struct Device *			USB2Base		= NULL;
struct USB2IFace *		IUSB2			= NULL;

// --

#define RTL8188EU_SYS_CFG 0x00f0

// --

S32
RTL8188EU_Probe( struct USB2_EPResource *control )
{
	U8 config[4];

	if ( ! RTL8188EU_RegisterIO( control, CMD_READ, RTL8188EU_SYS_CFG, config, sizeof( config ) ) )
	{
		USBERROR( "RTL8188EU: could not read the system configuration register" );

		return ( FALSE );
	}

	if ( ( config[0] == 0xff ) && ( config[1] == 0xff ) &&
		( config[2] == 0xff ) && ( config[3] == 0xff ) )
	{
		USBERROR( "RTL8188EU: invalid system configuration register" );

		return ( FALSE );
	}

	return ( TRUE );
}

// --
