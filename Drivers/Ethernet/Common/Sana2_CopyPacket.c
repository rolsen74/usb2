
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <exec/emulation.h>
#include <interfaces/exec.h>
#include <interfaces/utility.h>
#include "Sana2Commands.h"

// --

extern struct ExecIFace *IExec;
extern struct UtilityIFace *IUtility;

// --

int
Sana2_CopyPacket( struct IOSana2Req *ioreq, struct Hook *hook, APTR callback,
	ULONG method, APTR to, APTR from, uint32 length )
{
	struct SANA2CopyHookMsg copy;

	if ( hook )
	{
		copy.schm_Method = method;
		copy.schm_MsgSize = sizeof( copy );
		copy.schm_To = to;
		copy.schm_From = from;
		copy.schm_Size = length;

		return ( IUtility->CallHookPkt( hook, ioreq, & copy ) != 0 );
	}

	if ( ! callback )
	{
		return ( FALSE );
	}

	return ( IExec->EmulateTags( callback, ET_RegisterA0, to,
		ET_RegisterA1, from, ET_RegisterD0, length, TAG_END ) );
}

// --
