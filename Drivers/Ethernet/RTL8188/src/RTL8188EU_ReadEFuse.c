
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"

// --

/* Register sequence adapted from NetBSD if_urtwn.c. */
/*-
 * Copyright (c) 2010 Damien Bergamini <damien.bergamini@free.fr>
 * Copyright (c) 2014 Kevin Lo <kevlo@FreeBSD.org>
 * Copyright (c) 2016 Nathanial Sloss <nathanialsloss@yahoo.com.au>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

// --

S32
RTL8188EU_ReadEFuse( struct USB2_EPResource *control, U8 *rom, U32 size )
{
	U16 offset;
	U32 section;
	U32 word;
	U32 index;
	U8 header;
	U8 byte;

	// Enable the eFuse supply, loader and 8 MHz clock.
	if ( ( ! RTL8188EU_Update8( control, 0x0001, 0, 0x80 ) ) ||
		( ! RTL8188EU_Update8( control, RTL8188EU_SYS_FUNC_EN + 1, 0, 0x10 ) ) ||
		( ! RTL8188EU_Update8( control, 0x0008, 0, 0x22 ) ) )
	{
		return ( FALSE );
	}

	memset( rom, 0xff, size );

	for( offset = 0; offset < RTL8188EU_EFUSE_ADDRESS_LIMIT; )
	{
		if ( ! RTL8188EU_ReadEFuseByte( control, offset++, & header ) )
		{
			return ( FALSE );
		}

		if ( header == 0xff )
		{
			return ( TRUE );
		}

		if ( ( header & 0x1f ) == 0x0f )
		{
			section = header >> 5;

			if ( ! RTL8188EU_ReadEFuseByte( control, offset++, & header ) )
			{
				return ( FALSE );
			}

			section |= ( header & 0xf0 ) >> 1;
		}
		else
		{
			section = header >> 4;
		}

		for( word = 0; word < 4; word++ )
		{
			if ( header & ( 1U << word ) )
			{
				continue;
			}

			index = section * 8 + word * 2;

			if ( ! RTL8188EU_ReadEFuseByte( control, offset++, & byte ) )
			{
				return ( FALSE );
			}

			if ( index < size )
			{
				rom[index] = byte;
			}

			if ( ! RTL8188EU_ReadEFuseByte( control, offset++, & byte ) )
			{
				return ( FALSE );
			}

			if ( index + 1 < size )
			{
				rom[index + 1] = byte;
			}
		}
	}

	return ( FALSE );
}

// --
