
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Radio.h"

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

#define RTL8188EU_TX_BOUNDARY 170
#define RTL8188EU_TX_LAST_PAGE 176

// --

S32
RTL8188EU_InitDMA( struct USB2_EPResource *control, U32 endpoints )
{
	static const U16 boundary[] = { 0x0424, 0x0425, 0x045d, 0x0114, 0x0209 };

	U32 i;
	U8 next;
	U8 mapping[2];

	if ( ( ! endpoints ) || ( endpoints > 3 ) )
	{
		return ( FALSE );
	}

	for( i = 0; i <= RTL8188EU_TX_LAST_PAGE; i++ )
	{
		next = i + 1;

		if ( i == RTL8188EU_TX_BOUNDARY - 1 )
		{
			next = 0xff;
		}
		else if ( i == RTL8188EU_TX_LAST_PAGE )
		{
			next = RTL8188EU_TX_BOUNDARY;
		}

		if ( ! RTL8188EU_WriteLLT( control, i, next ) )
		{
			return ( FALSE );
		}
	}

	if ( ( ! RTL8188EU_Write16( control, 0x0214, 0 ) ) ||
		( ! RTL8188EU_Write16( control, 0x0214, 0x000d ) ) ||
		( ! RTL8188EU_Write32( control, 0x0200, 0x808e000d ) ) )
	{
		return ( FALSE );
	}

	for( i = 0; i < sizeof( boundary ) / sizeof( boundary[0] ); i++ )
	{
		if ( ! RTL8188EU_Write8( control, boundary[i], RTL8188EU_TX_BOUNDARY ) )
		{
			return ( FALSE );
		}
	}

	if ( ! RTL8188EU_RegisterIO( control, CMD_READ, 0x010c, mapping, sizeof( mapping ) ) )
	{
		return ( FALSE );
	}

	// Use the first bulk OUT pipe for all queues and keep RX aggregation disabled.
	return ( RTL8188EU_Write16( control, 0x010c, ( endpoints == 1 ? 0x5550 : 0xfff0 ) | ( mapping[0] & 0x0b ) ) &&
		RTL8188EU_Write16( control, 0x0116, 0x23ff ) &&
		RTL8188EU_Write8( control, 0x0104, 0x11 ) );
}

// --
