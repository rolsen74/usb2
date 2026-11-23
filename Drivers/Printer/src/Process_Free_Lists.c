
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static void
__myProcess_Free_Lists( struct PrinterUnit *unit )
{
	struct PrtBuffer *pb;
	struct IOStdReq *pario;

	USBERROR( "Printer : __myProcess_Free_Lists" );

	// --

	if ( unit->unit_USB_IOReq_FreeList )
	{
		USBINFO( "Printer : Freeing Tx Free List" );

//		Abort_IOList( unit, unit->unit_QueueList );
//		  Abort_BulkOut( unit, unit->unit_TxCurrentIO );

		FreeSysObject( ASOT_LIST, unit->unit_USB_IOReq_FreeList );

		unit->unit_USB_IOReq_FreeList = NULL;
	}

	// --

//	if ( unit->unit_Bulk_Tx_Active )
//	{
//		USBINFO( "Printer : Freeing Tx Active Array" );

//		Abort_IOList( unit, unit->unit_QueueList );
//		  Abort_BulkOut( unit, unit->unit_TxCurrentIO );

//		FreeSysObject( ASOT_LIST, unit->unit_Bulk_Tx_Active );
//		unit->unit_Bulk_Tx_Active = NULL;
//	}

	// --

	if ( unit->unit_PAR_IOReq_WaitingList )
	{
		USBINFO( "Printer : Freeing Tx Waiting List" );

		while(( pario = (PTR) RemHead( unit->unit_PAR_IOReq_WaitingList )))
		{
			pario->io_Actual = 0;
			pario->io_Error = ( pario->io_Flags & IOPARF_ABORT ) ? IOERR_ABORTED : ParErr_NotOpen;

			ReplyMsg( (PTR) pario );
		}

		FreeSysObject( ASOT_LIST, unit->unit_PAR_IOReq_WaitingList );

		unit->unit_PAR_IOReq_WaitingList = NULL;
	}

	// --

	if ( unit->unit_Buffer_Active )
	{
		FreeVec( unit->unit_Buffer_Active );

		unit->unit_Buffer_Active = NULL;
	}

	if ( unit->unit_Buffer_FreeList )
	{
		while(( pb = (PTR) RemHead( unit->unit_Buffer_FreeList )))
		{
			FreeVec( pb );
		}

		FreeSysObject( ASOT_LIST, unit->unit_Buffer_FreeList );

		unit->unit_Buffer_FreeList = NULL;
	}

	if ( unit->unit_Buffer_WaitingList )
	{
		while(( pb = (PTR) RemHead( unit->unit_Buffer_WaitingList )))
		{
			FreeVec( pb );
		}

		FreeSysObject( ASOT_LIST, unit->unit_Buffer_WaitingList );

		unit->unit_Buffer_WaitingList = NULL;
	}

	// --
}

// --
