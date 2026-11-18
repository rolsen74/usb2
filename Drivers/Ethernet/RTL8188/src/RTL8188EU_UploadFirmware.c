
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
RTL8188EU_UploadFirmware( struct USB2_EPResource *control, U8 *image, U32 length )
{
	U32 offset;
	U32 page;
	U32 remaining;
	U16 chunk;
	U8 status;

	if ( ( ! length ) || ( length > RTL8188EU_FW_MAX_SIZE ) )
	{
		return ( FALSE );
	}

	if ( ! RTL8188EU_RegisterIO( control, CMD_READ, RTL8188EU_FW_CTL, & status, sizeof( status ) ) )
	{
		return ( FALSE );
	}

	if ( status & RTL8188EU_FW_RAM_SELECTED )
	{
		if ( ( ! RTL8188EU_Write8( control, RTL8188EU_FW_CTL, 0 ) ) ||
			( ! RTL8188EU_ResetMCU( control ) ) )
		{
			return ( FALSE );
		}
	}

	if ( ( ! RTL8188EU_Update8( control, RTL8188EU_FW_CTL, 0, RTL8188EU_FW_ENABLED ) ) ||
		( ! RTL8188EU_Update8( control, RTL8188EU_FW_CTL + 2, 0x08, 0 ) ) ||
		( ! RTL8188EU_Update8( control, RTL8188EU_FW_CTL, 0, RTL8188EU_FW_CHECKSUM ) ) )
	{
		goto failed;
	}

	for( page = 0; length; page++ )
	{
		if ( ! RTL8188EU_Update8( control, RTL8188EU_FW_CTL + 2, RTL8188EU_FW_PAGE_MASK, page ) )
		{
			goto failed;
		}

		remaining = length < RTL8188EU_FW_PAGE_SIZE ? length : RTL8188EU_FW_PAGE_SIZE;

		for( offset = 0; remaining; offset += chunk )
		{
			// The vendor write request supports 196-byte, word and byte writes.
			chunk = remaining > 196 ? 196 : ( remaining > 4 ? 4 : 1 );

			if ( ! RTL8188EU_RegisterIO( control, CMD_WRITE, RTL8188EU_FW_RAM + offset, image, chunk ) )
			{
				goto failed;
			}

			remaining -= chunk;
			image += chunk;
			length -= chunk;
		}
	}

	if ( ( ! RTL8188EU_Update8( control, RTL8188EU_FW_CTL, RTL8188EU_FW_ENABLED, 0 ) ) ||
		( ! RTL8188EU_Write8( control, RTL8188EU_FW_CTL + 1, 0 ) ) ||
		( ! RTL8188EU_Wait8( control, RTL8188EU_FW_CTL, RTL8188EU_FW_CHECKSUM, RTL8188EU_FW_CHECKSUM ) ) )
	{
		goto failed;
	}

	if ( ( ! RTL8188EU_Update8( control, RTL8188EU_FW_CTL, RTL8188EU_FW_RUNNING, RTL8188EU_FW_READY ) ) ||
		( ! RTL8188EU_ResetMCU( control ) ) ||
		( ! RTL8188EU_Wait8( control, RTL8188EU_FW_CTL, RTL8188EU_FW_RUNNING, RTL8188EU_FW_RUNNING ) ) )
	{
		goto failed;
	}

	return ( TRUE );

failed:

	RTL8188EU_StopFirmware( control );

	return ( FALSE );
}

// --
