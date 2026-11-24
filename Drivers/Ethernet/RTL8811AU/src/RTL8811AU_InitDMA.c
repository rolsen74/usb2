
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

#define LLT_INIT       0x01e0
#define LLT_WRITE      0x40000000U
#define LLT_BUSY       0xc0000000U
#define TX_LAST_PAGE   243U
#define TX_RING_START  ( TX_LAST_PAGE + 1 )
#define TX_RING_END    254U
#define RX_BUFFER_END  0x3e7f

// --

S32
RTL8811AU_InitDMA( struct USB2_EPResource *control, U32 max_packet )
{
	static const U16 boundary[] = { 0x0424, 0x0425, 0x045d, 0x0114, 0x0209, 0x0457 };

	U32 index;
	U32 page;
	U32 next;
	U8 burst;

	for( page = 0; page <= TX_RING_END; page++ )
	{
		next = page + 1;

		if ( page == TX_LAST_PAGE )
		{
			next = 0xff;
		}
		else if ( page == TX_RING_END )
		{
			next = TX_RING_START;
		}

		if ( ( ! RTL8811AU_WriteReg( control, LLT_INIT, 4, LLT_WRITE | ( page << 8 ) | next ) ) ||
			( ! RTL8811AU_PollReg( control, LLT_INIT, 4, LLT_BUSY, 0, 10, NULL ) ) )
		{
			return ( FALSE );
		}
	}

	for( index = 0; index < sizeof( boundary ) / sizeof( boundary[0] ); index++ )
	{
		next = ( boundary[index] == 0x0457 ) ? TX_RING_START + 8 : TX_RING_START;

		if ( ! RTL8811AU_WriteReg( control, boundary[index], 1, next ) )
		{
			return ( FALSE );
		}
	}

	burst = ( max_packet >= 512 ) ? 0x1e : 0x2e;

	// TX remains paused. Use a single high queue and disable RX aggregation.
	return ( ( RTL8811AU_WriteReg( control, 0x0214, 1, 0 ) ) &&
		( RTL8811AU_WriteReg( control, 0x0200, 4, 0x80db0018 ) ) &&
		( RTL8811AU_UpdateReg( control, 0x010c, 2, 0xfff4, 0xfff0 ) ) &&
		( RTL8811AU_WriteReg( control, 0x0116, 2, RX_BUFFER_END ) ) &&
		( RTL8811AU_WriteReg( control, 0x0229, 1, TX_RING_START + 8 ) ) &&
		( RTL8811AU_Update8( control, 0x022a, 0, 0x02 ) ) &&
		( RTL8811AU_Update8( control, 0x0290, 0x3e, burst ) ) );
}

// --
