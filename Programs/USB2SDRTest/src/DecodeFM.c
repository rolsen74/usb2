
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <proto/exec.h>
#include "SDRTest.h"

// --

S32
SDR_DecodeFM( const char *inputPath, const char *outputPath )
{
	U8 *iq;
	S64 size;
	BPTR input;
	BPTR outputLock;
	BPTR inputLock;
	S32 same;
	S32 result;

	iq = NULL;
	result = RETURN_FAIL;

	input = Open( inputPath, MODE_OLDFILE );

	if ( ! input )
	{
		PrintFault( IoErr(), inputPath );

		return ( RETURN_FAIL );
	}

	outputLock = Lock( outputPath, ACCESS_READ );

	if ( outputLock )
	{
		inputLock = DupLockFromFH( input );

		same = ( ! inputLock ) || ( SameLock( inputLock, outputLock ) == LOCK_SAME );

		if ( inputLock )
		{
			UnLock( inputLock );
		}

		UnLock( outputLock );

		if ( same )
		{
			Printf( "Choose a separate file for the FM WAV output.\n" );

			goto bailout;
		}
	}

	size = GetFileSize( input );

	if ( ( size < 0 ) || ( size > 0x7fffffff ) || ( size & 1 ) )
	{
		Printf( "Expected raw unsigned 8-bit I/Q with complete sample pairs in %s.\n", inputPath );

		goto bailout;
	}

	if ( ! size )
	{
		Printf( "%s is too short to decode.\n", inputPath );

		goto bailout;
	}

	iq = AllocVecTags( (U32) size, TAG_END );

	if ( ! iq )
	{
		Printf( "Could not allocate the I/Q input buffer.\n" );

		goto bailout;
	}

	if ( Read( input, iq, size ) != size )
	{
		Printf( "Could not read all of %s.\n", inputPath );

		goto bailout;
	}

	Printf( "Decoding %s to mono FM audio...\n", inputPath );

	if ( SDR_SaveFM( outputPath, iq, (U32) size ) )
	{
		result = RETURN_OK;
	}

bailout:
	FreeVec( iq );

	Close( input );

	return ( result );
}

// --
