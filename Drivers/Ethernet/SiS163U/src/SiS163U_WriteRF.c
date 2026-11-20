
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define SIS163U_RF_CTRL	0x00fc

// --

S32
SiS163U_WriteRF( struct USB2_EPResource *control, U8 address, U16 value )
{
	U32 state;
	U32 command;
	U32 bits;
	U32 index;
	S32 error;

	error = SiS163U_ReadRegister( control, SIS163U_RF_CTRL, & state );

	if ( error )
	{
		return ( error );
	}

	// The type 8 RF serial engine shifts a 22-bit word in reverse order.
	bits = ( ( (U32) value << 5 ) | address ) << 1;
	command = 0;

	for( index = 0; index < 22; index++ )
	{
		command = ( command << 1 ) | ( bits & 1 );
		bits >>= 1;
	}

	command = ( command << 10 ) | ( state & 0xe0 ) | 0x16;

	error = SiS163U_WriteRegister( control, SIS163U_RF_CTRL, command );

	if ( error )
	{
		return ( error );
	}

	return ( SiS163U_WaitRegister( control, SIS163U_RF_CTRL, 0x10 ) );
}

// --
