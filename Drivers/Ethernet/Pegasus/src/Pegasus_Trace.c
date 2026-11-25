
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Pegasus.h"

// --

void
Pegasus_TraceRXStart( struct devUnit *unit, const struct USB2_IORequest *request )
{
	if ( ! unit->unit_PegasusRXStarted )
	{
		unit->unit_PegasusRXStarted = TRUE;

		IExec->DebugPrintF( "Pegasus: queued first bulk IN, %lu bytes\n", (U32) request->io_Length );
	}
}

// --

void
Pegasus_TraceBulk( struct devUnit *unit, const struct USB2_IORequest *request, S32 receive )
{
	const U8 *frame;

	U8 *count;
	S32 error;

	error = ( request->io_Error != USB2Err_NoError ) ||
		( ( ! receive ) && ( request->io_Actual != request->io_Length ) );

	if ( error )
	{
		count = receive ? & unit->unit_PegasusRXErrors : & unit->unit_PegasusTXErrors;

		if ( *count < 6 )
		{
			( *count )++;

			IExec->DebugPrintF( "Pegasus: bulk %s error %ld, %lu/%lu bytes\n",
				receive ? "IN" : "OUT", (S32) request->io_Error,
				(U32) request->io_Actual, (U32) request->io_Length );
		}
	}
	else if ( ( ! receive ) && ( unit->unit_PegasusTXLogged < 6 ) )
	{
		frame = (const U8 *) request->io_Data + WIRED_TX_HEADER_SIZE;
		unit->unit_PegasusTXLogged++;
		unit->unit_PegasusStatusPending = TRUE;

		IExec->DebugPrintF( "Pegasus: TX bulk OUT completed, %lu bytes, type $%04lx\n",
			(U32) request->io_Actual, ( (U32) frame[12] << 8 ) | frame[13] );
	}
}

// --

void
Pegasus_TraceMACStatus( struct devUnit *unit )
{
	U8 status[5];
	U32 missed;

	if ( ! unit->unit_PegasusStatusPending )
	{
		return;
	}

	unit->unit_PegasusStatusPending = FALSE;

	// The MAC status can describe several frames since the last sample.
	if ( Pegasus_Read( unit, PEG_TX_STATUS, status, sizeof( status ) ) )
	{
		missed = ( (U32) ( status[3] & 0x7f ) << 8 ) | status[4];

		IExec->DebugPrintF( "Pegasus: MAC status sample TX $%02lx/$%02lx, RX $%02lx, missed RX %lu\n",
			(U32) status[0], (U32) status[1], (U32) status[2], missed );
	}
	else
	{
		IExec->DebugPrintF( "Pegasus: could not read MAC packet status\n" );
	}
}

// --

void
Pegasus_TraceRXDelivery( struct devUnit *unit, U32 type, U32 result )
{
	if ( unit->unit_PegasusRXDeliveryLogged < 6 )
	{
		unit->unit_PegasusRXDeliveryLogged++;

		IExec->DebugPrintF( "Pegasus: SANA-II RX type $%04lx, matched %lu, copied %lu, copy failed %lu\n",
			type, (U32) ( ( result & SANA2_RX_TYPED ) != 0 ),
			(U32) ( ( result & SANA2_RX_COPIED ) != 0 ), (U32) ( ( result & SANA2_RX_COPY_FAILED ) != 0 ) );
	}
}

// --
