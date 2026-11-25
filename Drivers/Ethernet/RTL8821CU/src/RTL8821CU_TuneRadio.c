
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"
#include "Registers.h"

// --

/*
 * RTL8821C setup adapted from Linux rtw88 RTL8821C sources
 * under its BSD-3-Clause terms.
 *
 * Copyright (c) 2018-2019 Realtek Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// --

S32
RTL8821CU_TuneRadio( struct EthernetUnit *unit, U8 channel, S32 enable_tx )
{
	static const U32 swings[] = { 0x200, 0x16a, 0x101, 0x0b6 };

	struct USB2_EPResource *control;

	U32 rfe;
	U32 radio;
	S32 btg;

	control = unit->unit_USB_Register->Res_Control;
	unit->unit_TxReady = FALSE;
	rfe = unit->unit_RFOption & 0x1f;
	btg = ( rfe == 2 ) || ( rfe == 4 ) || ( rfe == 7 ) || ( rfe == 10 ) || ( rfe == 12 ) || ( rfe == 15 );

	if ( ( channel < 1 ) || ( channel > 11 ) )
	{
		return ( FALSE );
	}

	if ( ( ! RTL8821CU_WriteReg( control, 0x0522, 1, 0xff ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0808, 4, 0x10000000, 0x10000000 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0454, 1, 0x80, 0 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0a80, 4, 0x0004ffff, btg ? 0xfc84 : 0x7532 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0a84, 4, 0x00ff0000, btg ? 0x000e0000 : 0x00120000 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0814, 4, 0x0000fc00, 0x00003c00 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0c1c, 4, 0xffe00f00, swings[unit->unit_BBSwing / 3] << 21 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0860, 4, 0x1ffe0000, 0x12d40000 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x08ac, 4, 0x103103ff, 0x10010000 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x08c4, 4, 0x40000000, 0x40000000 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0668, 2, 0x0180, 0 ) ) ||
		( ! RTL8821CU_WriteReg( control, 0x0483, 1, 0 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x1080, 4, 0x00010000, 0x00010000 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0000, 4, 0x04000000, 0x04000000 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0cb8, 4, 0x00f50000, btg ? 0x00010000 : 0x00700000 ) ) ||
		( ! RTL8821CU_UpdateRF( control, 0xdf, 0x40, 0x40 ) ) ||
		( ! RTL8821CU_UpdateRF( control, 0x64, 0x0f, 0x0f ) ) ||
		( ! RTL8821CU_UpdateRF( control, 0x18, 0x70fff, 0x0c00 | channel ) ) ||
		( ! RTL8821CU_UpdateRF( control, 0xb8, 0x80000, 0 ) ) ||
		( ! RTL8821CU_UpdateRF( control, 0xb8, 0x80000, 0x80000 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0948, 4, 0x30000000, 0x20000000 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x094c, 4, 0x30000000, 0x20000000 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x0c20, 4, 0x80000000, 0x80000000 ) ) ||
		( ! RTL8821CU_UpdateReg( control, 0x08f0, 4, 0x80000000, 0 ) ) ||
		( ! RTL8821CU_ReadReg( control, 0x2860, 4, & radio ) ) )
	{
		return ( FALSE );
	}

	if ( ( radio & 0x70fff ) != ( 0x0c00U | channel ) )
	{
		DebugPrintF( "RTL8821CU: channel %lu RF readback $%08lx\n", (U32) channel, radio );

		return ( FALSE );
	}

	unit->unit_Channel = channel;

	DebugPrintF( "RTL8821CU: channel %lu at 20 MHz, RF $%05lx\n", (U32) channel, radio & 0x000fffff );

	return ( ( ! enable_tx ) || ( RTL8821CU_SetTxPower( unit ) ) );
}

// --
