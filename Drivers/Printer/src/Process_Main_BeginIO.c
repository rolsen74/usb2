
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

#include "cmd_0009_PDQuery.c"

static void __myProcess_Main_BeginIO( struct PrinterUnit *unit )
{
struct IORequest *ioreq;

	while( TRUE )
	{
		ioreq = (PTR) GetMsg( unit->unit_PRT_Begin_MsgPort );

		if ( ! ioreq )
		{
			break;
		}

		ioreq->io_Error = 0;

		switch( ioreq->io_Command )
		{
			case CMD_WRITE:
			{
				USBERROR( "BeginIO : CMD_WRITE" );

				if ( unit->unit_Buffer_Size )
				{
					Cmd_Write_Buffer( unit, (PTR) ioreq );
				}
				else
				{
					Cmd_Write_Direct( unit, (PTR) ioreq );
				}
				break;
			}

			case PDCMD_QUERY:
			{
				USBERROR( "BeginIO : PDCMD_QUERY" );

				Cmd_PDQuery( unit, (PTR) ioreq );
				break;
			}

			default:
			{
				USBINFO( "Unknown Command (%ld) -- This should really not chould happen", ioreq->io_Command );
				ioreq->io_Error = IOERR_NOCMD;
				ReplyMsg( (PTR) ioreq );
				break;
			}
		}
	}
}

// --
