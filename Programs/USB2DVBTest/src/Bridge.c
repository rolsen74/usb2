
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
** AF9015 protocol reference Copyright (C) 2007 Antti Palosaari
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "AF9015.h"
#include <string.h>

// --

enum
{
	GET_CONFIG = 0x10,
	DOWNLOAD_FIRMWARE = 0x11,
	BOOT = 0x13,
	READ_MEMORY = 0x20,
	WRITE_MEMORY = 0x21,
	READ_WRITE_I2C = 0x22,
	WRITE_VIRTUAL_MEMORY = 0x26
};

enum
{
	COMMAND_HEADER = 8,
	REPLY_HEADER = 2,
	MAX_MESSAGE = 63,
	EEPROM_ADDRESS = 0x50,
	DEMOD_ADDRESS = 0x1c,
	FIRMWARE_ADDRESS = 0x5100
};

// --

static BOOL
Command( struct Receiver *receiver, U8 code, U8 i2c, U16 reg, U8 mailbox, U8 addrSize,
	BOOL read, U8 size, const U8 *out, U8 *in )
{
	U8 *packet;
	U8 seq;
	U32 writeSize;
	U32 replySize;

	writeSize = COMMAND_HEADER + ( read ? 0 : size );
	replySize = REPLY_HEADER + ( read ? size : 0 );

	if ( ( writeSize > MAX_MESSAGE ) || ( replySize > MAX_MESSAGE ) )
	{
		return ( FALSE );
	}

	if ( AF9015_Detached( receiver ) )
	{
		return ( FALSE );
	}

	seq = receiver->Sequence++;
	packet = receiver->Out->Buffers[0];
	packet[0] = code;
	packet[1] = seq;
	packet[2] = i2c;
	packet[3] = reg >> 8;
	packet[4] = reg;
	packet[5] = mailbox;
	packet[6] = addrSize;
	packet[7] = size;

	if ( ( ! read ) && ( size ) )
	{
		memcpy( & packet[COMMAND_HEADER], out, size );
	}

	if ( ! AF9015_Transfer( receiver->Out, FALSE, writeSize, FALSE ) )
	{
		Printf( "AF9015 command $%02lx at $%04lx could not be sent.\n", (U32) code, (U32) reg );

		return ( FALSE );
	}

	if ( code == DOWNLOAD_FIRMWARE )
	{
		return ( TRUE );
	}

	if ( ! AF9015_Transfer( receiver->In, TRUE, replySize, FALSE ) )
	{
		Printf( "AF9015 command $%02lx at $%04lx received no valid reply.\n", (U32) code, (U32) reg );

		return ( FALSE );
	}

	packet = receiver->In->Buffers[0];

	if ( ( packet[0] != seq ) || ( packet[1] ) )
	{
		Printf( "AF9015 command $%02lx at $%04lx: reply sequence $%02lx (expected $%02lx), status $%02lx.\n",
			(U32) code, (U32) reg, (U32) packet[0], (U32) seq, (U32) packet[1] );

		return ( FALSE );
	}

	if ( read )
	{
		memcpy( in, & packet[REPLY_HEADER], size );
	}

	return ( TRUE );
}

// --

BOOL
AF9015_Read( struct Receiver *receiver, U16 reg, U8 *data, U8 size )
{
	if ( ( ! size ) || ( size > 16 ) )
	{
		return ( FALSE );
	}

	if ( ( reg >> 8 ) == 0xdd )
	{
		return ( Command( receiver, READ_MEMORY, 0, reg, 0, 0, TRUE, size, NULL, data ) );
	}

	return ( Command( receiver, READ_MEMORY, DEMOD_ADDRESS << 1, reg,
		( ( size - 1 ) << 2 ) | 2, 3, TRUE, size, NULL, data ) );
}

// --

BOOL
AF9015_Write( struct Receiver *receiver, U16 reg, const U8 *data, U8 size )
{
	U8 code;

	if ( ( reg >> 8 ) == 0xdd )
	{
		return ( Command( receiver, WRITE_MEMORY, 0, reg, 0, 0, FALSE, size, data, NULL ) );
	}

	// The AF9013 command mailbox limits ordinary bursts to 16 registers.
	while( size )
	{
		code = WRITE_MEMORY;

		if ( ( ( reg >> 8 ) == 0xff ) || ( ( reg >> 8 ) == 0xae ) )
		{
			code = WRITE_VIRTUAL_MEMORY;
		}

		if ( ( reg >> 8 ) == 0xae )
		{
			if ( ! Command( receiver, code, DEMOD_ADDRESS << 1, reg, 3, 3, FALSE, 1, data, NULL ) )
			{
				return ( FALSE );
			}

			reg++;
			data++;
			size--;
		}
		else
		{
			return ( ( size <= 16 ) && ( Command( receiver, code, DEMOD_ADDRESS << 1, reg,
				( ( size - 1 ) << 2 ) | 3, 3, FALSE, size, data, NULL ) ) );
		}
	}

	return ( TRUE );
}

// --

BOOL
AF9015_Set( struct Receiver *receiver, U16 reg, U8 mask, U8 value )
{
	U8 old;
	U8 next;

	old = 0;

	if ( ( mask != 0xff ) && ( ! AF9015_Read( receiver, reg, & old, 1 ) ) )
	{
		return ( FALSE );
	}

	next = ( old & ~mask ) | ( value & mask );

	return ( ( ( mask != 0xff ) && ( old == next ) ) || ( AF9015_Write( receiver, reg, & next, 1 ) ) );
}

// --

BOOL
AF9015_Table( struct Receiver *receiver, const struct RegisterSetting *table, U32 count )
{
	U32 index;

	for( index = 0; index < count; index++ )
	{
		if ( ! AF9015_Set( receiver, table[index].Address, table[index].Mask, table[index].Value ) )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

BOOL
AF9015_I2CRead( struct Receiver *receiver, U8 chip, U8 reg, U8 *data, U8 size )
{
	return ( Command( receiver, READ_WRITE_I2C, ( chip << 1 ) | 1, reg, 0, 1, TRUE, size, NULL, data ) );
}

// --

BOOL
AF9015_I2CWrite( struct Receiver *receiver, U8 chip, U8 reg, const U8 *data, U8 size )
{
	U16 address;
	U8 mailbox;
	U8 prefix;

	if ( ( ! size ) || ( size > 20 ) )
	{
		return ( FALSE );
	}

	address = ( reg << 8 ) | data[0];
	mailbox = 0;
	prefix = 1;

	if ( size > 1 )
	{
		mailbox = data[1];
		prefix = 2;
	}

	// USB firmware counts the register and up to two data bytes as the I2C prefix.
	return ( Command( receiver, READ_WRITE_I2C, chip << 1, address, mailbox, 1 + prefix, FALSE,
		size - prefix, data + prefix, NULL ) );
}

// --

BOOL
AF9015_Firmware( struct Receiver *receiver, const char *path )
{
	BPTR file;
	LONG size;
	int64 fileSize;
	U32 address;
	U16 lastBlock;
	U8 state;
	U8 chunk[MAX_MESSAGE - COMMAND_HEADER];
	BOOL ok;

	if ( ! Command( receiver, GET_CONFIG, 0, 0, 0, 0, TRUE, 1, NULL, & state ) )
	{
		return ( FALSE );
	}

	if ( state == 2 )
	{
		return ( TRUE );
	}

	file = Open( path, MODE_OLDFILE );

	if ( ! file )
	{
		Printf( "AF9015 firmware is required. Could not open %s.\n", path );

		return ( FALSE );
	}

	fileSize = GetFileSize( file );

	if ( ( fileSize <= 0 ) || ( fileSize > 0x10000 - FIRMWARE_ADDRESS ) )
	{
		Close( file );

		Printf( "Invalid AF9015 firmware size.\n" );

		return ( FALSE );
	}

	address = FIRMWARE_ADDRESS;
	lastBlock = FIRMWARE_ADDRESS;
	ok = FALSE;

	while( ( size = Read( file, chunk, sizeof( chunk ) ) ) > 0 )
	{
		if ( ( address + size > 0x10000 ) || ( CheckSignal( SIGBREAKF_CTRL_C ) ) ||
			( ! Command( receiver, DOWNLOAD_FIRMWARE, 0, address, 0, 0, FALSE, size, chunk, NULL ) ) )
		{
			break;
		}

		lastBlock = address;
		address += size;
	}

	Close( file );

	if ( ( ! size ) && ( address - FIRMWARE_ADDRESS == fileSize ) &&
		( Command( receiver, BOOT, 0, lastBlock, 0, 0, FALSE, 0, NULL, NULL ) ) )
	{
		Delay( 5 );

		ok = Command( receiver, GET_CONFIG, 0, 0, 0, 0, TRUE, 1, NULL, & state ) && ( state == 2 );
	}

	if ( ! ok )
	{
		Printf( "AF9015 firmware did not start.\n" );
	}

	return ( ok );
}

// --

BOOL
AF9015_Board( struct Receiver *receiver )
{
	static const U32 clocks[] = { 28800000, 20480000, 28000000, 25000000 };

	U8 eeprom[12];
	U8 version[4];
	U32 index;
	U32 attempt;

	// The first EEPROM access can time out on cold hardware.
	for( attempt = 0; attempt < 4; attempt++ )
	{
		if ( AF9015_I2CRead( receiver, EEPROM_ADDRESS, 0x31, eeprom, 1 ) )
		{
			break;
		}
	}

	if ( attempt == 4 )
	{
		return ( FALSE );
	}

	for( index = 1; index < sizeof( eeprom ); index++ )
	{
		if ( ! AF9015_I2CRead( receiver, EEPROM_ADDRESS, 0x31 + index, & eeprom[index], 1 ) )
		{
			return ( FALSE );
		}
	}

	if ( ( eeprom[5] >= sizeof( clocks ) / sizeof( clocks[0] ) ) ||
		( ! AF9015_Read( receiver, 0x5103, version, sizeof( version ) ) ) )
	{
		Printf( "AF9015 could not determine the demodulator clock or firmware revision.\n" );

		return ( FALSE );
	}

	receiver->Dual = eeprom[0];
	receiver->ClockType = eeprom[5];
	receiver->Clock = clocks[receiver->ClockType];
	receiver->IFrequency = ( ( eeprom[8] << 8 ) | eeprom[7] ) * 1000U;
	receiver->MT2060_IF1 = ( eeprom[10] << 8 ) | eeprom[9];
	receiver->Tuner = eeprom[11];

	Printf( "AF9015 firmware %lu.%lu.%lu.%lu, tuner ID $%02lx, crystal %lu Hz, IF %lu Hz, TS mode $%02lx.\n",
		(U32) version[0], (U32) version[1], (U32) version[2], (U32) version[3],
		(U32) receiver->Tuner, receiver->Clock, receiver->IFrequency, (U32) receiver->Dual );

	return ( TRUE );
}

// --
