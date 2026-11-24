
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"

// --

/*-
 * Copyright (c) 2016 Andriy Voskoboinyk <avos@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

// --

S32
RTL8811AU_SetChannel( struct USB2_EPResource *control, const struct RTL8811AU_Board *board, U8 channel )
{
	U32 pinmux;
	U32 inversion;
	U32 channel_rf;

	if ( ( channel < 1 ) || ( channel > 11 ) )
	{
		return ( FALSE );
	}

	pinmux = board->ExternalLNA2GHz ? 0x7272 : 0x7777;
	inversion = board->ExternalLNA2GHz ? 0x00100000 : 0;

	if ( ( ! RTL8811AU_UpdateReg( control, 0x0808, 4, 0, 0x30000000 ) ) ||
		( ! RTL8811AU_UpdateReg( control, 0x0cb0, 4, 0x0000f5f5, pinmux ) ) ||
		( ! RTL8811AU_UpdateReg( control, 0x0cb4, 4, 0x00500000, inversion ) ) ||
		( ! RTL8811AU_UpdateReg( control, 0x0c1c, 4, 0x00000f00, 0 ) ) ||
		( ! RTL8811AU_UpdateReg( control, 0x080c, 4, 0xf0, 0x10 ) ) ||
		( ! RTL8811AU_UpdateReg( control, 0x0a04, 4, 0x0f000000, 0x01000000 ) ) ||
		( ! RTL8811AU_WriteReg( control, 0x0454, 1, 0 ) ) ||
		( ! RTL8811AU_UpdateReg( control, 0x0860, 4, 0x1ffe0000, 0x12d40000 ) ) ||
		( ! RTL8811AU_UpdateReg( control, 0x0668, 2, 0x0180, 0 ) ) ||
		( ! RTL8811AU_WriteReg( control, 0x0483, 1, 0 ) ) ||
		( ! RTL8811AU_UpdateReg( control, 0x08ac, 4, 0x003003c3, 0x00300200 ) ) ||
		( ! RTL8811AU_UpdateReg( control, 0x08c4, 4, 0x40000000, 0 ) ) ||
		( ! RTL8811AU_UpdateReg( control, 0x0848, 4, 0x03c00000, 0x02000000 ) ) ||
		( ! RTL8811AU_ReadRF( control, 0x18, &channel_rf ) ) )
	{
		return ( FALSE );
	}

	channel_rf = ( channel_rf & ~0x00070fffU ) | 0x0c00 | channel;

	if ( ( ! RTL8811AU_WriteRF( control, 0x18, channel_rf ) ) ||
		( ! RTL8811AU_ReadRF( control, 0x18, &channel_rf ) ) )
	{
		return ( FALSE );
	}

	if ( ( channel_rf & 0x00070fff ) != ( 0x0c00U | channel ) )
	{
		DebugPrintF( "RTL8811AU: RF channel readback mismatch, channel %lu, register $%05lx\n",
			(U32) channel, channel_rf );

		return ( FALSE );
	}

	DebugPrintF( "RTL8811AU: RF channel %lu at 20 MHz, register $%05lx\n", (U32) channel, channel_rf );

	return ( TRUE );
}

// --
