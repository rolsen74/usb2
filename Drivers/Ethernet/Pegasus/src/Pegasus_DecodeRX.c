
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define PEG_RX_TRAILER_SIZE 4
#define PEG_RX_ERROR_MASK 0x1c
#define PEG_RX_LENGTH_MASK 0x0fff
#define PEG_RX_PREFIX_SIZE 2
#define ETHERNET_HEADER_SIZE 14
#define ETHERNET_FCS_SIZE 4

// --

static S32
RejectFrame( struct devUnit *unit, U32 size, const U8 *status )
{
	if ( unit->unit_PegasusRXRejected < 6 )
	{
		unit->unit_PegasusRXRejected++;

		if ( status )
		{
			IExec->DebugPrintF( "Pegasus: discarded RX transfer %lu bytes, trailer $%02lx/$%02lx/$%02lx/$%02lx\n",
				size, (U32) status[0], (U32) status[1], (U32) status[2], (U32) status[3] );
		}
		else
		{
			IExec->DebugPrintF( "Pegasus: discarded short RX transfer %lu bytes\n", size );
		}
	}

	return ( FALSE );
}

// --

S32
Wired_DecodeRX( struct devUnit *unit, U8 *buffer, U32 size )
{
	const U8 *status;

	U32 length;
	U32 offset;

	if ( ! size )
	{
		return ( TRUE );
	}

	if ( size < PEG_RX_TRAILER_SIZE )
	{
		return ( RejectFrame( unit, size, NULL ) );
	}

	status = buffer + size - PEG_RX_TRAILER_SIZE;

	if ( status[2] & PEG_RX_ERROR_MASK )
	{
		return ( RejectFrame( unit, size, status ) );
	}

	if ( unit->unit_PegasusRXPrefix )
	{
		offset = PEG_RX_PREFIX_SIZE;
		length = ( buffer[0] | ( (U32) buffer[1] << 8 ) ) & PEG_RX_LENGTH_MASK;
	}
	else
	{
		offset = 0;
		length = ( status[0] | ( (U32) status[1] << 8 ) ) & PEG_RX_LENGTH_MASK;

		// The trailing length counts the Ethernet FCS even when it is stripped.
		if ( length < ETHERNET_FCS_SIZE )
		{
			return ( RejectFrame( unit, size, status ) );
		}

		length -= ETHERNET_FCS_SIZE;
	}

	if ( ( length < ETHERNET_HEADER_SIZE ) || ( length > ETHERNET_MTU ) ||
		( offset + length + PEG_RX_TRAILER_SIZE > size ) )
	{
		return ( RejectFrame( unit, size, status ) );
	}

	if ( unit->unit_PegasusRXLogged < 6 )
	{
		unit->unit_PegasusRXLogged++;

		IExec->DebugPrintF( "Pegasus: RX frame %lu bytes, type $%04lx (USB %lu bytes)\n",
			length, ( (U32) buffer[offset + 12] << 8 ) | buffer[offset + 13], size );
	}

	ReadPacket( unit, buffer + offset, length );

	return ( TRUE );
}

// --
