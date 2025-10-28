
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "All.h"

// --

void Cmd_Write_Buffer( struct PrinterUnit *unit, struct IOExtPar *pario )
{
S32 stat;
S32 len;
STR str;

	USBERROR( "Printer : Cmd_Write_Buffer : pario %p : length %ld", pario, pario->IOPar.io_Length );

	/**/ if ( pario->IOPar.io_Length == -1U )
	{
		str = pario->IOPar.io_Data;
		len = strlen( str );
		stat = Buffer_Write( unit, (PTR) str, len );

		USBERROR( "Printer : Cmd_Write_Buffer : String : Stat %ld", stat );
	}
	else if ( pario->IOPar.io_Length > 0 )
	{
		len = pario->IOPar.io_Length;
		stat = Buffer_Write( unit, pario->IOPar.io_Data, len );

		USBERROR( "Printer : Cmd_Write_Buffer : Data : Stat %ld", stat );
	}
	else
	{
		len = 0;
		stat = FALSE;

		USBERROR( "Printer : Cmd_Write_Buffer : Invalid IOReq Length %ld", pario->IOPar.io_Length );
	}

	if ( stat )
	{
		USBERROR( "Printer : Cmd_Write_Buffer : Okay : Len %ld", len );

		pario->IOPar.io_Actual = len;
		pario->IOPar.io_Error = 0;
		pario->io_Status = 0;
	}
	else
	{
		USBERROR( "Printer : Cmd_Write_Buffer : Error : Len %ld", len );

		pario->IOPar.io_Actual = 0;
		pario->IOPar.io_Error = ParErr_LineErr;
		pario->io_Status = 0;
	}

	ReplyMsg( (PTR) pario );
}

// --
