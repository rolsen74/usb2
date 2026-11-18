
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

S32
RTL8188EU_StartRadio( struct EthernetUnit *unit )
{
	struct USB2_EPResource *control;

	U32 rf_env;
	U32 crystal;
	U8 saved[4];

	control = unit->unit_USB_Register->Res_Control;
	crystal = unit->unit_CrystalCap;

	if ( ( ! RTL8188EU_InitMAC( control ) ) ||
		( ! RTL8188EU_Update8( control, RTL8188EU_SYS_FUNC_EN + 1, 0, 0x20 ) ) ||
		( ! RTL8188EU_Write8( control, 0x001f, 0x07 ) ) ||
		( ! RTL8188EU_Write8( control, RTL8188EU_SYS_FUNC_EN, 0x17 ) ) ||
		( ! RTL8188EU_InitBaseband( control ) ) ||
		( ! RTL8188EU_Update32( control, 0x0024, 0x007ff800, ( crystal | ( crystal << 6 ) ) << 11 ) ) ||
		( ! RTL8188EU_RegisterIO( control, CMD_READ, 0x0870, saved, sizeof( saved ) ) ) )
	{
		return ( FALSE );
	}

	rf_env = saved[0] | ( (U32) saved[1] << 8 );

	if ( ( ! RTL8188EU_Update32( control, 0x0860, 0, 0x00100010 ) ) ||
		( ! RTL8188EU_Update32( control, 0x0824, 0x00000c00, 0 ) ) ||
		( ! RTL8188EU_InitRF( control ) ) ||
		( ! RTL8188EU_Update32( control, 0x0870, 0x0000ffff, rf_env ) ) ||
		( ! RTL8188EU_SetChannel( control, 1 ) ) )
	{
		return ( FALSE );
	}

	if ( ! ( RTL8188EU_Write8( control, 0x0522, 0xff ) &&
		RTL8188EU_RegisterIO( control, CMD_WRITE, 0x0610, unit->unit_StationAddress, 6 ) &&
		RTL8188EU_Write8( control, 0x0680, 0 ) &&
		RTL8188EU_Write8( control, 0x060f, 4 ) &&
		RTL8188EU_Write32( control, 0x0608, 0x1000200e ) &&
		RTL8188EU_Write16( control, 0x06a0, 0xffff ) &&
		RTL8188EU_Write16( control, 0x06a2, 0 ) &&
		RTL8188EU_Write16( control, 0x06a4, 0 ) &&
		RTL8188EU_Update32( control, 0x0800, 0, 0x03000000 ) &&
		RTL8188EU_Update8( control, 0x0100, 0x40, 0x80 ) ) )
	{
		return ( FALSE );
	}

	return ( RTL8188EU_TuneRadio( unit, 1 ) );
}

// --
