/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE struct SerialBase *Serial_Manager_Init( struct SerialBase *base, PTR seglist, struct ExecIFace *IExec UNUSED )
{
	base->Library.lib_Node.ln_Type = NT_DEVICE;
	base->Library.lib_Node.ln_Pri = 0;
	base->Library.lib_Node.ln_Name = (STR) SERIAL_DEVICE_NAME;
	base->Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	base->Library.lib_Version = 1;
	base->Library.lib_Revision = 0;
	base->Library.lib_IdString = NULL;
	base->SegList = seglist;

	return ( base );
}

// --
