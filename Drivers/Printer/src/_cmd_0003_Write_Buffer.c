
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
	S32 stat;
	S32 len;
	STR str;

	USBERROR( "Printer : Cmd_Write_Buffer : pario %p : length %ld", pario, pario->io_Length );

	/**/ if ( ! pario->io_Data )
	{
		len = 0;
		stat = FALSE;
	}
	else if ( pario->io_Length == -1U )
	{
		str = pario->io_Data;

		len = strlen( str );

		stat = Buffer_Write( unit, (PTR) str, len );

		USBERROR( "Printer : Cmd_Write_Buffer : String : Stat %ld", stat );
	}
	else if ( pario->io_Length > 0 )
	{
		len = pario->io_Length;

		stat = Buffer_Write( unit, pario->io_Data, len );

		USBERROR( "Printer : Cmd_Write_Buffer : Data : Stat %ld", stat );
	}
	else
	{
		len = 0;
		stat = FALSE;

		USBERROR( "Printer : Cmd_Write_Buffer : Invalid IOReq Length %ld", pario->io_Length );
	}

	if ( stat )
	{
		USBERROR( "Printer : Cmd_Write_Buffer : Okay : Len %ld", len );

		pario->io_Actual = len;
		pario->io_Error = 0;
	}
	else
	{
		USBERROR( "Printer : Cmd_Write_Buffer : Error : Len %ld", len );

		pario->io_Actual = 0;
		pario->io_Error = ParErr_LineErr;
	}

	ReplyMsg( (PTR) pario );
}

// --
