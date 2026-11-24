
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Registers.h"

// --

#define EFUSE_WIFI_LIMIT ( 512 - 96 )
#define EFUSE_READ_READY 0x80000000U
#define EFUSE_ADDRESS_DATA 0x0003ffffU
#define EFUSE_ADDRESS_SHIFT 8
#define EFUSE_WIFI_BANK_MASK 0x0300
#define EFUSE_WRITE_VOLTAGE 0x80

// --

static S32
ReadEFuseByte( struct USB2_EPResource *control, U32 *offset, U8 *byte )
{
	U32 value;

	if ( ( *offset >= EFUSE_WIFI_LIMIT ) ||
		( ! RTL8821CU_UpdateReg( control, REG_EFUSE_CTRL, 4, EFUSE_READ_READY | EFUSE_ADDRESS_DATA, *offset << EFUSE_ADDRESS_SHIFT ) ) ||
		( ! RTL8821CU_PollReg( control, REG_EFUSE_CTRL, 4, EFUSE_READ_READY, EFUSE_READ_READY ) ) ||
		( ! RTL8821CU_ReadReg( control, REG_EFUSE_CTRL, 1, & value ) ) )
	{
		return ( FALSE );
	}

	*byte = value;
	*offset += 1;

	return ( TRUE );
}

// --

S32
RTL8821CU_ReadEFuse( struct USB2_EPResource *control, U8 *map, U32 size )
{
	U32 offset;
	U32 section;
	U32 word;
	U32 index;
	U8 header;
	U8 low;
	U8 high;

	if ( ( ! RTL8821CU_UpdateReg( control, REG_LDO_EFUSE_CTRL, 4, EFUSE_WIFI_BANK_MASK, 0 ) ) ||
		( ! RTL8821CU_UpdateReg( control, REG_LDO_EFUSE_CTRL + 3, 1, EFUSE_WRITE_VOLTAGE, 0 ) ) )
	{
		return ( FALSE );
	}

	memset( map, 0xff, size );

	offset = 0;

	while( offset < EFUSE_WIFI_LIMIT )
	{
		if ( ! ReadEFuseByte( control, & offset, & header ) )
		{
			return ( FALSE );
		}

		if ( header == 0xff )
		{
			break;
		}

		if ( ( header & 0x1f ) == 0x0f )
		{
			section = header >> 5;

			if ( ! ReadEFuseByte( control, & offset, & header ) )
			{
				return ( FALSE );
			}

			section |= ( header & 0xf0 ) >> 1;
		}
		else
		{
			section = header >> 4;
		}

		for( word = 0; word < 4; word++ )
		{
			if ( header & ( 1U << word ) )
			{
				continue;
			}

			if ( ( ! ReadEFuseByte( control, & offset, & low ) ) ||
				( ! ReadEFuseByte( control, & offset, & high ) ) )
			{
				return ( FALSE );
			}

			index = section * 8 + word * 2;

			if ( ( index < size ) )
			{
				map[index] = low;
			}

			index++;

			if ( ( index < size ) )
			{
				map[index] = high;
			}
		}
	}

	return ( TRUE );
}

// --
