
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#include "cmd_0009_PDQuery.c"

static S32
__myProcess_Main_BeginIO( struct PrinterUnit *unit )
{
	struct IORequest *ioreq;
	struct IOExtPar *pario;

	U32 index;

	for ( index = 0 ; index < 32 ; index++ )
	{
		ioreq = (PTR) GetMsg( unit->unit_PRT_Begin_MsgPort );

		if ( ! ioreq )
		{
			return ( TRUE );
		}

		ioreq->io_Error = 0;

		if ( ioreq->io_Message.mn_Length < sizeof( struct IOStdReq ) )
		{
			ioreq->io_Error = IOERR_BADLENGTH;

			Printer_Reply( (PTR) ioreq );

			continue;
		}

		if ( ioreq->io_Message.mn_Length >= sizeof( struct IOExtPar ) )
		{
			pario = (PTR) ioreq;
			pario->io_Status = 0;
		}

		if ( ( unit->unit_Detached ) || ( ioreq->io_Flags & IOPARF_ABORT ) )
		{
			ioreq->io_Error = ( ioreq->io_Flags & IOPARF_ABORT ) ? IOERR_ABORTED : ParErr_NotOpen;

			Printer_Reply( (PTR) ioreq );

			continue;
		}

		switch( ioreq->io_Command )
		{
			case CMD_WRITE:
			{
				if ( unit->unit_JobError )
				{
					ioreq->io_Error = unit->unit_JobError;

					Printer_Reply( (PTR)ioreq );

					break;
				}

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

				if ( ioreq->io_Message.mn_Length >= sizeof( struct IOExtPar ) )
				{
					Cmd_PDQuery( unit, (PTR) ioreq );
				}
				else
				{
					ioreq->io_Error = IOERR_BADLENGTH;

					Printer_Reply( (PTR) ioreq );
				}

				break;
			}

			default:
			{
				USBINFO( "Unknown Command (%ld) -- This should really not chould happen", ioreq->io_Command );

				ioreq->io_Error = IOERR_NOCMD;

				Printer_Reply( (PTR) ioreq );

				break;
			}
		}
	}

	Signal( FindTask( NULL ), unit->unit_PRT_Begin_MsgPortBit );

	return ( FALSE );
}

// --
