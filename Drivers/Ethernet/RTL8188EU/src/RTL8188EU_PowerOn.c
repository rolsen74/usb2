
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

#define RTL8188EU_APS_FSMCO 0x0004
#define RTL8188EU_LDO 0x0023
#define RTL8188EU_XTAL 0x0024
#define RTL8188EU_CR 0x0100

// --

S32
RTL8188EU_PowerOn( struct USB2_EPResource *control )
{
	U8 disabled[2];
	U8 enabled[2];

	disabled[0] = 0;
	disabled[1] = 0;
	enabled[0] = 0x3f;
	enabled[1] = 0x06;

	if ( ( ! RTL8188EU_Wait8( control, RTL8188EU_APS_FSMCO + 2, 0x02, 0x02 ) ) ||
		( ! RTL8188EU_Update8( control, RTL8188EU_SYS_FUNC_EN, 0x03, 0 ) ) ||
		( ! RTL8188EU_Update8( control, RTL8188EU_XTAL + 2, 0, 0x80 ) ) ||
		( ! RTL8188EU_Update8( control, RTL8188EU_APS_FSMCO + 1, 0x98, 0 ) ) ||
		( ! RTL8188EU_Update8( control, RTL8188EU_APS_FSMCO + 1, 0, 0x01 ) ) ||
		( ! RTL8188EU_Wait8( control, RTL8188EU_APS_FSMCO + 1, 0x01, 0 ) ) ||
		( ! RTL8188EU_Update8( control, RTL8188EU_LDO, 0x10, 0 ) ) ||
		( ! RTL8188EU_RegisterIO( control, CMD_WRITE, RTL8188EU_CR, disabled, sizeof( disabled ) ) ) )
	{
		return ( FALSE );
	}

	// Enable the DMA, protocol, scheduler, security and calibration blocks.
	return ( RTL8188EU_RegisterIO( control, CMD_WRITE, RTL8188EU_CR, enabled, sizeof( enabled ) ) );
}

// --
