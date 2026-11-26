
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "AF9015.h"

// --

#define CAPTURE_READS 4

// --

static BOOL
StreamOn( struct Receiver *receiver )
{
	static const U8 blockWords[] = { ( AF9015_TS_BLOCK / 4 ) & 0xff, ( AF9015_TS_BLOCK / 4 ) >> 8 };
	const struct RegisterSetting bridge[] =
	{
		{ 0xdd0c, 0xff, 512 / 4 },
		{ 0xd50b, 0x01, receiver->Dual ? 0x01 : 0x00 },
		{ 0xd520, 0x10, receiver->Dual ? 0x10 : 0x00 },
		{ 0xdd13, 0x20, 0x00 },
		{ 0xdd11, 0x20, 0x20 },
		{ 0xd507, 0x04, 0x00 }
	};

	return ( ( AF9015_Write( receiver, 0xdd88, blockWords, sizeof( blockWords ) ) ) &&
		( AF9015_Table( receiver, bridge, sizeof( bridge ) / sizeof( bridge[0] ) ) ) );
}

// --

static void
StreamOff( struct Receiver *receiver )
{
	AF9015_Set( receiver, 0xd507, 0x04, 0x04 );

	AF9015_Set( receiver, 0xdd11, 0x20, 0 );

	AF9015_Set( receiver, 0xdd13, 0x20, 0x20 );
}

// --

static void
QueueRead( struct USB2_EPResource *bulk, U32 slot )
{
	struct USB2_IORequest *io;

	io = bulk->IORequests[slot];
	io->io_Command = CMD_READ;
	io->io_Data = bulk->Buffers[slot];
	io->io_Length = AF9015_TS_BLOCK;
	io->io_TimeOut = 2000000;
	io->io_AllowShortPackets = FALSE;
	io->io_Actual = 0;

	SendIO( (PTR) io );
}

// --

static BOOL
WritePackets( BPTR file, const U8 *data )
{
	U32 offset;

	for( offset = 0; offset < AF9015_TS_BLOCK; offset += AF9015_TS_PACKET )
	{
		if ( data[offset] != 0x47 )
		{
			Printf( "AF9015 lost transport-stream synchronization.\n" );

			return ( FALSE );
		}
	}

	if ( file )
	{
		if ( Write( file, data, AF9015_TS_BLOCK ) != AF9015_TS_BLOCK )
		{
			PrintFault( IoErr(), "USB2DVBTest output" );

			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

static U32
CaptureBlocks( struct Receiver *receiver, struct USB2_EPResource *bulk, BPTR file, U32 blocks )
{
	struct USB2_IORequest *io;

	U32 active;
	U32 slot;
	U32 submitted;
	U32 received;
	U32 signals;
	U32 waitMask;
	S32 error;

	slot = 0;
	active = 0;
	received = 0;
	waitMask = bulk->MsgPortBit | receiver->Reg->Stack_MsgPortBit | SIGBREAKF_CTRL_C;

	for( submitted = 0; submitted < MIN( blocks, CAPTURE_READS ); submitted++ )
	{
		QueueRead( bulk, submitted );

		active |= 1U << submitted;
	}

	if ( ! StreamOn( receiver ) )
	{
		goto bailout;
	}

	while( received < blocks )
	{
		if ( CheckSignal( SIGBREAKF_CTRL_C ) )
		{
			break;
		}

		if ( AF9015_Detached( receiver ) )
		{
			Printf( "AF9015 receiver disconnected.\n" );

			break;
		}

		io = bulk->IORequests[slot];

		if ( ! CheckIO( (PTR) io ) )
		{
			signals = Wait( waitMask );

			if ( signals & SIGBREAKF_CTRL_C )
			{
				break;
			}

			continue;
		}

		error = WaitIO( (PTR) io );

		active &= ~( 1U << slot );

		if ( ( error ) || ( io->io_Actual != AF9015_TS_BLOCK ) )
		{
			Printf( "AF9015 transport read failed (%ld, %lu/%lu bytes).\n", error,
				io->io_Actual, (U32) AF9015_TS_BLOCK );

			break;
		}

		if ( ! WritePackets( file, bulk->Buffers[slot] ) )
		{
			break;
		}

		received++;

		if ( submitted < blocks )
		{
			QueueRead( bulk, slot );

			active |= 1U << slot;
			submitted++;
		}

		slot = ( slot + 1 ) % CAPTURE_READS;
	}

bailout:
	for( slot = 0; slot < CAPTURE_READS; slot++ )
	{
		if ( active & ( 1U << slot ) )
		{
			io = bulk->IORequests[slot];

			if ( ! CheckIO( (PTR) io ) )
			{
				AbortIO( (PTR) io );
			}
		}
	}

	for( slot = 0; slot < CAPTURE_READS; slot++ )
	{
		if ( active & ( 1U << slot ) )
		{
			WaitIO( (PTR) bulk->IORequests[slot] );
		}
	}

	return ( received );
}

// --

BOOL
AF9015_Capture( struct Receiver *receiver, const char *path, U32 blocks )
{
	struct USB2_EPResource *bulk;

	BPTR file;
	BPTR existing;
	U32 captured;
	BOOL closed;

	file = 0;

	if ( path )
	{
		existing = Lock( path, ACCESS_READ );

		if ( existing )
		{
			UnLock( existing );

			Printf( "Output file already exists. Choose a new path.\n" );

			return ( FALSE );
		}

		if ( IoErr() == ERROR_OBJECT_NOT_FOUND )
		{
			file = Open( path, MODE_NEWFILE );
		}

		if ( ! file )
		{
			PrintFault( IoErr(), "USB2DVBTest output" );

			return ( FALSE );
		}
	}

	bulk = USB2_EPRes_ObtainTags( receiver->Reg, USB2Tag_EPRes_InterfaceNr, AF9015_INTERFACE,
		USB2Tag_EPRes_EndPointNr, AF9015_TS_IN, USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_NrOfIORequest, CAPTURE_READS, USB2Tag_EPRes_BufferSize, AF9015_TS_BLOCK, TAG_END );
	captured = 0;

	if ( bulk )
	{
		captured = CaptureBlocks( receiver, bulk, file, blocks );

		if ( ! AF9015_Detached( receiver ) )
		{
			StreamOff( receiver );
		}

		USB2_EPRes_Release( bulk );
	}
	else
	{
		Printf( "Could not obtain AF9015 transport endpoint $84.\n" );
	}

	closed = TRUE;

	if ( file )
	{
		closed = Close( file );

		if ( ! closed )
		{
			PrintFault( IoErr(), "USB2DVBTest output" );
		}
	}

	Printf( "Received %lu MPEG-TS packets in %lu of %lu blocks.\n", captured * 87, captured, blocks );

	return ( ( captured == blocks ) && ( closed ) );
}

// --
