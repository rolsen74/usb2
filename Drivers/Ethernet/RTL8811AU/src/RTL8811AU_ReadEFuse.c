
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define EFUSE_ACCESS 0x00cf
#define EFUSE_ENABLE 0x69
#define EFUSE_READY  0x80000000U
#define EFUSE_ADDR   0x0003ff00U
#define EFUSE_LIMIT  512U

// --

static S32
ReadNext( struct USB2_EPResource *control, U32 *offset, U8 *byte )
{
	U32 status;

	if ( *offset >= EFUSE_LIMIT )
	{
		return ( FALSE );
	}

	if ( ! RTL8811AU_ReadReg( control, RTL8811AU_EFUSE_CTRL, 4, &status ) )
	{
		return ( FALSE );
	}

	status = ( status & ~( EFUSE_READY | EFUSE_ADDR ) ) | ( *offset << 8 );

	if ( ( ! RTL8811AU_WriteReg( control, RTL8811AU_EFUSE_CTRL, 4, status ) ) ||
		( ! RTL8811AU_PollReg( control, RTL8811AU_EFUSE_CTRL, 4, EFUSE_READY, EFUSE_READY, 10, &status ) ) )
	{
		return ( FALSE );
	}

	*byte = status;
	*offset += 1;

	return ( TRUE );
}

// --

static S32
ReadMap( struct USB2_EPResource *control, U8 *rom, U32 size )
{
	U32 offset;
	U32 section;
	U32 word;
	U32 index;
	U8 header;

	offset = 0;

	for( index = 0; index < size; index++ )
	{
		rom[index] = 0xff;
	}

	while( ReadNext( control, &offset, &header ) )
	{
		if ( header == 0xff )
		{
			return ( TRUE );
		}

		section = header >> 4;

		if ( ( header & 0x1f ) == 0x0f )
		{
			section = header >> 5;

			if ( ! ReadNext( control, &offset, &header ) )
			{
				return ( FALSE );
			}

			section |= ( header & 0xf0 ) >> 1;
		}

		for( word = 0; word < 4; word++ )
		{
			if ( header & ( 1U << word ) )
			{
				continue;
			}

			index = section * 8 + word * 2;

			if ( ( index + 1 >= size ) || ( ! ReadNext( control, &offset, &rom[index] ) ) ||
				( ! ReadNext( control, &offset, &rom[index + 1] ) ) )
			{
				return ( FALSE );
			}
		}
	}

	return ( FALSE );
}

// --

S32
RTL8811AU_ReadEFuse( struct USB2_EPResource *control, U8 *rom, U32 size )
{
	S32 read;
	S32 closed;

	if ( ! RTL8811AU_WriteReg( control, EFUSE_ACCESS, 1, EFUSE_ENABLE ) )
	{
		return ( FALSE );
	}

	read = ( RTL8811AU_Update8( control, RTL8811AU_SYS_FUNC + 1, 0, 0x10 ) ) &&
		( RTL8811AU_Update8( control, RTL8811AU_SYS_CLOCK, 0, 0x22 ) ) &&
		( ReadMap( control, rom, size ) );

	closed = RTL8811AU_WriteReg( control, EFUSE_ACCESS, 1, 0 );

	return ( read && closed );
}

// --
