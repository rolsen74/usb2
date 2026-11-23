
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
Cmd_Write_Buffer( struct PrinterUnit *unit, struct IOStdReq *pario )
{
	U32 len;
	U32 accepted;

	if ( ( ! pario->io_Data ) || ( ! pario->io_Length ) )
	{
		pario->io_Error = ParErr_InvParam;

		ReplyMsg( (PTR)pario );

		return;
	}

	len = ( pario->io_Length == -1U ) ? strlen( pario->io_Data ) : pario->io_Length;

	accepted = Buffer_Write( unit, pario->io_Data, len );

	pario->io_Actual = accepted;
	pario->io_Error = ( accepted == len ) ? 0 : ParErr_LineErr;

	ReplyMsg( (PTR)pario );
}

// --
