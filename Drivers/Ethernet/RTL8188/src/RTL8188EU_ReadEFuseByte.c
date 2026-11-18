
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
RTL8188EU_ReadEFuseByte( struct USB2_EPResource *control, U16 offset, U8 *value )
{
	U8 reg[4];

	if ( ( offset >= RTL8188EU_EFUSE_ADDRESS_LIMIT ) ||
		( ! RTL8188EU_RegisterIO( control, CMD_READ, RTL8188EU_EFUSE_CTRL, reg, sizeof( reg ) ) ) )
	{
		return ( FALSE );
	}

	reg[1] = offset;
	reg[2] = ( reg[2] & ~3U ) | ( offset >> 8 );
	reg[3] &= 0x7f;

	if ( ( ! RTL8188EU_RegisterIO( control, CMD_WRITE, RTL8188EU_EFUSE_CTRL, reg, sizeof( reg ) ) ) ||
		( ! RTL8188EU_Wait8( control, RTL8188EU_EFUSE_CTRL + 3, 0x80, 0x80 ) ) )
	{
		return ( FALSE );
	}

	return ( RTL8188EU_RegisterIO( control, CMD_READ, RTL8188EU_EFUSE_CTRL, value, 1 ) );
}

// --
