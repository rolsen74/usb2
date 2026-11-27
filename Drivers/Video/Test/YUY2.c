
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

static inline void
WriteRGB( U8 *rgb, U8 y, S32 red, S32 green, S32 blue )
{
	S32 luma;

	luma = 298 * ( y - 16 );
	rgb[0] = ClampComponent( ( luma + red ) >> 8 );
	rgb[1] = ClampComponent( ( luma + green ) >> 8 );
	rgb[2] = ClampComponent( ( luma + blue ) >> 8 );
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
	S32 cb;
	S32 cr;
	S32 red;
	S32 green;
	S32 blue;

	if ( ( ! width ) || ( ! height ) || ( width & 1 ) ||
		( ! outWidth ) || ( ! outHeight ) || ( (U64) width * height * 2 != size ) )
	{
		return ( FALSE );
	}

	if ( ( width == outWidth ) && ( height == outHeight ) )
	{
		// Each YUY2 pair shares chroma. The full-size preview needs no scaling.
		for ( pixel = 0; pixel < size; pixel += 4 )
		{
			pair = frame + pixel;
			cb = pair[1] - 128;
			cr = pair[3] - 128;
			red = 409 * cr + 128;
			green = 128 - 100 * cb - 208 * cr;
			blue = 516 * cb + 128;

			WriteRGB( rgb, pair[0], red, green, blue );

			WriteRGB( rgb + 3, pair[2], red, green, blue );

			rgb += 6;
		}

		return ( TRUE );
	}

	for ( y = 0; y < outHeight; y++ )
	{
		row = frame + ( y * height / outHeight ) * width * 2;

		for ( x = 0; x < outWidth; x++ )
		{
			pixel = x * width / outWidth;
			pair = row + ( pixel & ~1U ) * 2;
			cb = pair[1] - 128;
			cr = pair[3] - 128;

			WriteRGB( rgb, pair[( pixel & 1 ) * 2], 409 * cr + 128, 128 - 100 * cb - 208 * cr, 516 * cb + 128 );

			rgb += 3;
		}
	}

	return ( TRUE );
}

// --
