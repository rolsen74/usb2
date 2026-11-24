/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE struct VideoBase *Video_Manager_Init( struct VideoBase *base, PTR seglist, struct ExecIFace *IExec UNUSED )
{
	base->Library.lib_Node.ln_Type = NT_DEVICE;
	base->Library.lib_Node.ln_Name = (STR) VIDEO_DEVICE_NAME;
	base->Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	base->Library.lib_Version = VIDEO_DEVICE_VERSION;
	base->Library.lib_Revision = VIDEO_DEVICE_REVISION;
	base->Library.lib_IdString = (STR) VIDEO_DEVICE_ID;
	base->SegList = seglist;

	return ( base );
}

// --
