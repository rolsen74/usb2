
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

static U32
CCKGain( U32 base )
{
	// rtw8821c_bb_pg_type0, path A, 1/2/5.5/11 Mbps in table order.
	static const S32 rate_power[] = { 38, 36, 34, 32 };
	// Lowest 20 MHz CCK limit across the upstream domains and channels 1-11.
	static const S32 world_limit = 28;

	U32 gain;
	U32 rate;
	S32 reference;
	S32 limit;
	S32 offset;
	S32 index;

	gain = 0;
	reference = rate_power[3];
	limit = world_limit - reference;

	for( rate = 0; rate < sizeof( rate_power ) / sizeof( rate_power[0] ); rate++ )
	{
		offset = rate_power[rate] - reference;
		index = (S32) base + ( offset < limit ? offset : limit );

		if ( index < 0 )
		{
			index = 0;
		}
		else if ( index > 0x3f )
		{
			index = 0x3f;
		}

		gain |= (U32) index << ( rate * 8 );
	}

	return ( gain );
}

// --

S32
RTL8821CU_SetTxPower( struct EthernetUnit *unit )
{
	struct USB2_EPResource *control;

	U32 channel;
	U32 group;
	U32 power;
	U32 gain;

	control = unit->unit_USB_Register->Res_Control;
	channel = unit->unit_Channel;
	group = channel <= 2 ? 0 : ( channel <= 5 ? 1 : ( channel <= 8 ? 2 : 3 ) );
	power = unit->unit_CCKPower[group];

	if ( power > 0x3f )
	{
		DebugPrintF( "RTL8821CU: channel %lu receive-only, invalid CCK power index $%02lx\n",
			(U32) channel, power );

		return ( TRUE );
	}

	gain = CCKGain( power );

	if ( ( ! RTL8821CU_WriteReg( control, 0x1d00, 4, gain ) ) ||
		( ! RTL8821CU_WriteReg( control, 0x0522, 1, 0 ) ) )
	{
		return ( FALSE );
	}

	unit->unit_TxReady = TRUE;

	DebugPrintF( "RTL8821CU: TX enabled on channel %lu, CCK power index $%02lx\n", channel, gain & 0xff );

	return ( TRUE );
}

// --
