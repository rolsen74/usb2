
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define FW_PATH     "DEVS:Firmware/rtw8821a_fw.bin"
#define FW_HEADER   32
#define FW_MAX_SIZE 32768

// --

S32
RTL8811AU_LoadFirmware( struct USB2_EPResource *control )
{
	BPTR file;
	U8 *image;
	S64 length;
	U32 signature;
	U32 payload;
	S32 loaded;

	loaded = FALSE;
	image = NULL;

	file = Open( FW_PATH, MODE_OLDFILE );

	if ( ! file )
	{
		DebugPrintF( "RTL8811AU: could not open %s, DOS error %ld\n", FW_PATH, IoErr() );

		return ( FALSE );
	}

	length = GetFileSize( file );

	if ( ( length <= FW_HEADER ) || ( length > FW_HEADER + FW_MAX_SIZE ) )
	{
		DebugPrintF( "RTL8811AU: invalid firmware file size\n" );

		goto bailout;
	}

	image = AllocVecTags( (U32) length, TAG_END );

	if ( ! image )
	{
		goto bailout;
	}

	if ( Read( file, image, (S32) length ) != length )
	{
		goto bailout;
	}

	signature = image[0] | ( (U32) image[1] << 8 );
	payload = image[12] | ( (U32) image[13] << 8 );

	if ( ( ( signature >> 4 ) != 0x210 ) || ( payload != length - FW_HEADER ) )
	{
		DebugPrintF( "RTL8811AU: invalid firmware header ($%04lx, %lu bytes)\n", signature, payload );

		goto bailout;
	}

	loaded = RTL8811AU_UploadFirmware( control, image + FW_HEADER, payload );

bailout:

	if ( image )
	{
		FreeVec( image );
	}

	Close( file );

	return ( loaded );
}

// --
