
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <devices/usb2.h>
#include <devices/timer.h>
#include "../inc/VideoDevice.h"
#include "Preview.h"

// --

static U8
ClampComponent( S32 value )
{
	return ( MIN( 255, MAX( 0, value ) ) );
}

// --

S32
VideoTest_YUY2ToRGB( const U8 *frame, U32 size, U32 width, U32 height,
	U8 *rgb, U32 outWidth, U32 outHeight )
{
	const U8 *row;
	const U8 *pair;

	U32 x;
	U32 y;
	U32 pixel;
	S32 luma;
	S32 cb;
	S32 cr;

	if ( ( ! width ) || ( ! height ) || ( width & 1 ) ||
		( ! outWidth ) || ( ! outHeight ) || ( (U64) width * height * 2 != size ) )
	{
		return ( FALSE );
	}

	for ( y = 0; y < outHeight; y++ )
	{
		row = frame + ( y * height / outHeight ) * width * 2;

		for ( x = 0; x < outWidth; x++ )
		{
			pixel = x * width / outWidth;
			pair = row + ( pixel & ~1U ) * 2;
			luma = 298 * ( pair[( pixel & 1 ) * 2] - 16 );
			cb = pair[1] - 128;
			cr = pair[3] - 128;

			// YUY2 stores one Cb and Cr sample for each pair of luma samples.
			*rgb++ = ClampComponent( ( luma + 409 * cr + 128 ) >> 8 );
			*rgb++ = ClampComponent( ( luma - 100 * cb - 208 * cr + 128 ) >> 8 );
			*rgb++ = ClampComponent( ( luma + 516 * cb + 128 ) >> 8 );
		}
	}

	return ( TRUE );
}

// --
