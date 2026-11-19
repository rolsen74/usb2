
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdlib.h>
#include <string.h>
#include <proto/dos.h>
#include <devices/usb2.h>
#include "../inc/VideoDevice.h"
#include "../../Audio/Test/Wav.h"
#include "PreviewAvi.h"

// --

#define AVI_HEADER_BYTES 224
#define AVI_MOVI_OFFSET 220
#define AVI_CLOCK_RATE 10000000U
#define AVI_FILE_LIMIT 0x7fffffffU

struct PreviewAviIndex
{
	U32 Offset;
	U32 Bytes;
};

// --

static void
PutLE32( U8 *data, U32 value )
{
	data[0] = value;
	data[1] = value >> 8;
	data[2] = value >> 16;
	data[3] = value >> 24;
}

// --

static S32
WriteHeader( const struct PreviewAvi *avi, const struct VideoFormat *format, U32 end, S32 indexed )
{
	const char *codec;

	U8 header[AVI_HEADER_BYTES];
	U64 measured;
	U64 maxRate;
	U32 interval;

	codec = ( format->Format == VIDEO_FORMAT_YUY2 ) ? "YUY2" : "MJPG";
	interval = format->FrameInterval;

	if ( ( avi->Frames > 1 ) && ( avi->LastTime > avi->FirstTime ) )
	{
		measured = ( avi->LastTime - avi->FirstTime ) * 10 / ( avi->Frames - 1 );

		if ( ( measured ) && ( measured <= 0xffffffffU ) )
		{
			interval = measured;
		}
	}

	maxRate = (U64) avi->LargestFrame * AVI_CLOCK_RATE / interval;

	memset( header, 0, sizeof( header ) );

	memcpy( header, "RIFF", 4 );

	PutLE32( header + 4, end - 8 );

	memcpy( header + 8, "AVI LIST", 8 );

	PutLE32( header + 16, 192 );

	memcpy( header + 20, "hdrlavih", 8 );

	PutLE32( header + 28, 56 );
	PutLE32( header + 32, MAX( 1, ( (U64) interval + 5 ) / 10 ) );
	PutLE32( header + 36, MIN( maxRate, 0xffffffffU ) );
	PutLE32( header + 44, indexed ? 0x10 : 0 );
	PutLE32( header + 48, avi->Frames );
	PutLE32( header + 56, 1 );
	PutLE32( header + 60, avi->LargestFrame );
	PutLE32( header + 64, format->Width );
	PutLE32( header + 68, format->Height );

	memcpy( header + 88, "LIST", 4 );

	PutLE32( header + 92, 116 );

	memcpy( header + 96, "strlstrh", 8 );

	PutLE32( header + 104, 56 );

	memcpy( header + 108, "vids", 4 );

	memcpy( header + 112, codec, 4 );

	PutLE32( header + 128, interval );
	PutLE32( header + 132, AVI_CLOCK_RATE );
	PutLE32( header + 140, avi->Frames );
	PutLE32( header + 144, avi->LargestFrame );
	PutLE32( header + 148, 0xffffffffU );

	Audio_WriteLE16( header + 160, format->Width );

	Audio_WriteLE16( header + 162, format->Height );

	memcpy( header + 164, "strf", 4 );

	PutLE32( header + 168, 40 );
	PutLE32( header + 172, 40 );
	PutLE32( header + 176, format->Width );
	PutLE32( header + 180, format->Height );

	Audio_WriteLE16( header + 184, 1 );

	Audio_WriteLE16( header + 186, format->Format == VIDEO_FORMAT_YUY2 ? 16 : 24 );

	memcpy( header + 188, codec, 4 );

	PutLE32( header + 192, format->Format == VIDEO_FORMAT_YUY2 ? format->MaxFrameSize : avi->LargestFrame );

	memcpy( header + 212, "LIST", 4 );

	PutLE32( header + 216, avi->End - AVI_MOVI_OFFSET );

	memcpy( header + AVI_MOVI_OFFSET, "movi", 4 );

	return ( Write( avi->File, header, sizeof( header ) ) == sizeof( header ) );
}

// --

static S32
WriteIndex( const struct PreviewAvi *avi, const struct VideoFormat *format )
{
	U8 buffer[512];
	U32 index;
	U32 used;
	const char *chunk;

	chunk = ( format->Format == VIDEO_FORMAT_YUY2 ) ? "00db" : "00dc";

	memcpy( buffer, "idx1", 4 );

	PutLE32( buffer + 4, avi->Frames * 16 );

	if ( Write( avi->File, buffer, 8 ) != 8 )
	{
		return ( FALSE );
	}

	used = 0;

	for ( index = 0; index < avi->Frames; index++ )
	{
		memcpy( buffer + used, chunk, 4 );

		PutLE32( buffer + used + 4, 0x10 );
		PutLE32( buffer + used + 8, avi->Index[index].Offset );
		PutLE32( buffer + used + 12, avi->Index[index].Bytes );
		used += 16;

		if ( used == sizeof( buffer ) )
		{
			if ( Write( avi->File, buffer, used ) != (S32) used )
			{
				return ( FALSE );
			}

			used = 0;
		}
	}

	return ( ( ! used ) || ( Write( avi->File, buffer, used ) == (S32) used ) );
}

// --

S32
PreviewAvi_Write( struct PreviewAvi *avi, const char *path, const struct VideoFormat *format,
	const U8 *frame, U32 bytes, U64 time )
{
	struct PreviewAviIndex *index;

	U8 chunk[8];
	U8 pad;
	U32 capacity;
	U32 extent;

	if ( ! format->FrameInterval )
	{
		return ( FALSE );
	}

	if ( ( ! bytes ) || ( bytes > format->MaxFrameSize ) ||
		( ( format->Format == VIDEO_FORMAT_YUY2 ) && ( bytes != format->MaxFrameSize ) ) ||
		( ( format->Format == VIDEO_FORMAT_MJPEG ) && ( ( bytes < 2 ) || ( frame[0] != 0xff ) || ( frame[1] != 0xd8 ) ) ) )
	{
		return ( TRUE );
	}

	extent = 8 + bytes + ( bytes & 1 );

	if ( (U64) MAX( avi->End, AVI_HEADER_BYTES ) + extent + 8 + ( (U64) avi->Frames + 1 ) * 16 > AVI_FILE_LIMIT )
	{
		Printf( "AVI recording reached the file size limit.\n" );

		return ( FALSE );
	}

	if ( avi->Frames == avi->Capacity )
	{
		capacity = avi->Capacity ? avi->Capacity * 2 : 256;

		index = realloc( avi->Index, capacity * sizeof( *index ) );

		if ( ! index )
		{
			Printf( "Could not allocate the AVI frame index.\n" );

			return ( FALSE );
		}

		avi->Index = index;
		avi->Capacity = capacity;
	}

	if ( ! avi->File )
	{
		avi->File = Open( path, MODE_NEWFILE );

		if ( ! avi->File )
		{
			Printf( "Could not open %s for AVI recording.\n", path );

			return ( FALSE );
		}

		avi->End = AVI_HEADER_BYTES;

		if ( ! WriteHeader( avi, format, avi->End, FALSE ) )
		{
			Printf( "Could not write AVI headers to %s.\n", path );

			return ( FALSE );
		}
	}

	pad = 0;

	memcpy( chunk, ( format->Format == VIDEO_FORMAT_YUY2 ) ? "00db" : "00dc", 4 );

	PutLE32( chunk + 4, bytes );

	if ( ( Write( avi->File, chunk, sizeof( chunk ) ) != sizeof( chunk ) ) ||
		( Write( avi->File, frame, bytes ) != (S32) bytes ) ||
		( ( bytes & 1 ) && ( Write( avi->File, & pad, 1 ) != 1 ) ) )
	{
		Printf( "AVI frame write failed for %s.\n", path );

		return ( FALSE );
	}

	if ( ! avi->Frames )
	{
		avi->FirstTime = time;
	}

	avi->LastTime = time;
	avi->Index[avi->Frames].Offset = avi->End - AVI_MOVI_OFFSET;
	avi->Index[avi->Frames].Bytes = bytes;
	avi->LargestFrame = MAX( avi->LargestFrame, bytes );
	avi->Frames++;
	avi->End += extent;

	return ( TRUE );
}

// --

void
PreviewAvi_Close( struct PreviewAvi *avi, const char *path, const struct VideoFormat *format )
{
	U32 end;
	S32 indexed;
	S32 saved;

	if ( avi->File )
	{
		indexed = ( ChangeFilePosition( avi->File, avi->End, OFFSET_BEGINNING ) ) &&
			( WriteIndex( avi, format ) );

		end = indexed ? avi->End + 8 + avi->Frames * 16 : avi->End;

		if ( GetFileSize( avi->File ) != end )
		{
			ChangeFileSize( avi->File, end, OFFSET_BEGINNING );
		}

		saved = ( GetFileSize( avi->File ) == end ) &&
			( ChangeFilePosition( avi->File, 0, OFFSET_BEGINNING ) ) &&
			( WriteHeader( avi, format, end, indexed ) );

		if ( ! Close( avi->File ) )
		{
			saved = FALSE;
		}

		if ( saved )
		{
			Printf( "Saved %lu video frames to %s.\n", (unsigned long) avi->Frames, path );
		}
		else
		{
			Printf( "Could not finalize %s.\n", path );
		}
	}

	free( avi->Index );

	*avi = (struct PreviewAvi) { 0 };
}

// --
