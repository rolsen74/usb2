/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE struct AudioBase *Audio_Manager_Init( struct AudioBase *base, PTR seglist, struct ExecIFace *IExec UNUSED )
{
	base->Library.lib_Node.ln_Type = NT_DEVICE;
	base->Library.lib_Node.ln_Name = (STR) AUDIO_DEVICE_NAME;
	base->Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	base->Library.lib_Version = 1;
	base->Library.lib_Revision = 0;
	base->SegList = seglist;

	return ( base );
}

// --
