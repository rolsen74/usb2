
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
** Realtek epatch and download protocol based on Linux btrtl
** Copyright (C) 2015 Endless Mobile, Inc.
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <interfaces/dos.h>
#include "All.h"

// --

#define RTL_FW_PATH "DEVS:Firmware/rtl8761bu_fw.bin"
#define RTL_CFG_PATH "DEVS:Firmware/rtl8761bu_config.bin"
#define RTL_FW_LIMIT ( 256U * 1024 )
#define RTL_CFG_LIMIT 4096U
#define RTL_FRAGMENT 252U

// --

static U16
ReadLE16( const U8 *data )
{
	return ( (U16) data[0] | ( (U16) data[1] << 8 ) );
}

static U32
ReadLE32( const U8 *data )
{
	return ( (U32) ReadLE16( data ) | ( (U32) ReadLE16( data + 2 ) << 16 ) );
}

// --

static S32
Command( struct BTUSB *usb, U8 *command, U32 length, U8 *reply, U32 reply_size )
{
	const U8 *chunk;

	U8 event[257];
	U32 used;
	U32 count;
	U32 event_size;
	U32 bytes;
	U32 index;

	if ( BTUSB_StartCommand( usb, command, length ) )
	{
		return ( FALSE );
	}

	used = 0;
	event_size = 2;

	for( count = 0; count < 16; count++ )
	{
		if ( BTUSB_ReadEventChunk( usb, & chunk, & bytes ) )
		{
			return ( FALSE );
		}

		for( index = 0; index < bytes; index++ )
		{
			event[used++] = chunk[index];

			if ( used != event_size )
			{
				continue;
			}

			if ( ( used == 2 ) && ( event_size == 2 ) && ( event[1] ) )
			{
				event_size = 2U + event[1];
				continue;
			}

			if ( ( used >= 6 ) && ( event[0] == 0x0e ) &&
				( event[3] == command[0] ) && ( event[4] == command[1] ) )
			{
				if ( ( event[5] ) || ( used != 6 + reply_size ) )
				{
					return ( FALSE );
				}

				if ( reply_size )
				{
					CopyMem( event + 6, reply, reply_size );
				}

				return ( TRUE );
			}

			used = 0;
			event_size = 2;
		}
	}

	return ( FALSE );
}

// --

static S32
ReadFirmware( struct DOSIFace *dos, const char *path, U8 *data, U32 limit, S32 optional )
{
	BPTR file;
	S32 count;
	U32 length;

	file = dos->Open( path, MODE_OLDFILE );

	if ( ! file )
	{
		return ( ( ( optional ) && ( dos->IoErr() == ERROR_OBJECT_NOT_FOUND ) ) ? 0 : -1 );
	}

	length = 0;

	do
	{
		count = dos->Read( file, data + length, limit + 1 - length );

		if ( count <= 0 )
		{
			break;
		}

		length += count;
	}
	while( length <= limit );

	dos->Close( file );

	return ( ( ( count < 0 ) || ( ! length ) || ( length > limit ) ) ? -1 : (S32) length );
}

// --

static U32
FindPatch( const U8 *data, U32 length, U8 rom, U32 *offset )
{
	const U8 signature[] = { 'R', 'e', 'a', 'l', 't', 'e', 'c', 'h' };
	const U8 trailer[] = { 0x51, 0x04, 0xfd, 0x77 };
	U32 count;
	U32 table_end;
	U32 cursor;
	U32 index;
	U32 size;
	U32 location;
	U8 opcode;
	U8 field_length;
	S32 project_found;

	if ( length < 14 + 8 + 3 + sizeof( trailer ) )
	{
		return ( 0 );
	}

	for( index = 0; index < sizeof( signature ); index++ )
	{
		if ( data[index] != signature[index] )
		{
			return ( 0 );
		}
	}

	cursor = length - sizeof( trailer );

	for( index = 0; index < sizeof( trailer ); index++ )
	{
		if ( data[cursor + index] != trailer[index] )
		{
			return ( 0 );
		}
	}

	count = ReadLE16( data + 12 );
	table_end = 14 + 8 * count;
	project_found = FALSE;

	while( cursor >= table_end + 3 )
	{
		opcode = data[--cursor];
		field_length = data[--cursor];

		if ( ( opcode == 0xff ) || ( ! field_length ) || ( field_length > cursor - table_end ) )
		{
			break;
		}

		cursor -= field_length;

		if ( ( opcode == 0 ) && ( field_length == 1 ) )
		{
			project_found = ( data[cursor] == 14 ); // RTL8761B family
			break;
		}
	}

	if ( ! project_found )
	{
		return ( 0 );
	}

	for( index = 0; index < count; index++ )
	{
		if ( ReadLE16( data + 14 + 2 * index ) == (U32) rom + 1 )
		{
			size = ReadLE16( data + 14 + 2 * count + 2 * index );
			location = ReadLE32( data + 14 + 4 * count + 4 * index );

			if ( ( size < 4 ) || ( location < table_end ) || ( location > cursor ) || ( size > cursor - location ) )
			{
				return ( 0 );
			}

			*offset = location;

			return ( size );
		}
	}

	return ( 0 );
}

// --

static S32
Download( struct BTUSB *usb, const U8 *patch, U32 length )
{
	U8 command[4 + RTL_FRAGMENT];
	U8 reply;
	U8 index;
	U32 bytes;

	command[0] = 0x20;
	command[1] = 0xfc;
	index = 0;

	for( ;; )
	{
		bytes = ( length < RTL_FRAGMENT ) ? length : RTL_FRAGMENT;
		command[2] = bytes + 1;
		command[3] = index | ( ( bytes < RTL_FRAGMENT ) ? 0x80 : 0 );

		if ( bytes )
		{
			CopyMem( (PTR) patch, command + 4, bytes );
		}

		if ( ( SetSignal( 0, 0 ) & SIGBREAKF_CTRL_C ) ||
			( ! Command( usb, command, bytes + 4, & reply, 1 ) ) ||
			( ( reply & 0x7f ) != index ) )
		{
			return ( FALSE );
		}

		if ( bytes < RTL_FRAGMENT )
		{
			return ( TRUE );
		}

		length -= bytes;
		patch += bytes;
		index = ( index == 0x7f ) ? 1 : index + 1;
	}
}

// --

S32
BTUSB_InitializeRealtek( struct BTUSB *usb, U16 vendor, U16 product )
{
	struct Library *dos_base;
	struct DOSIFace *dos;

	U8 version_command[] = { 0x01, 0x10, 0 };
	U8 rom_command[] = { 0x6d, 0xfc, 0 };
	U8 version[8];
	U8 rom;
	U8 *data;
	U8 *config;
	S32 length;
	S32 config_size;
	S32 result;
	U32 offset;
	U32 patch_size;

	if ( ! ( ( ( vendor == 0x0b05 ) && ( product == 0x190e ) ) ||
		( ( vendor == 0x2357 ) && ( product == 0x0604 ) ) ||
		( ( vendor == 0x0bda ) && ( product == 0x8771 ) ) ) )
	{
		return ( TRUE );
	}

	if ( ( ! Command( usb, version_command, sizeof( version_command ), version, sizeof( version ) ) ) ||
		( ReadLE16( version + 4 ) != 93 ) )
	{
		DebugPrintF( "Bluetooth USB: RTL8761BU controller version query failed\n" );

		return ( FALSE );
	}

	if ( ReadLE16( version + 6 ) != 0x8761 )
	{
		return ( TRUE ); // The controller already runs a firmware patch.
	}

	if ( ( version[0] != 0x0a ) || ( ReadLE16( version + 1 ) != 0x000b ) ||
		( ! Command( usb, rom_command, sizeof( rom_command ), & rom, 1 ) ) )
	{
		DebugPrintF( "Bluetooth USB: unsupported RTL8761BU ROM revision\n" );

		return ( FALSE );
	}

	dos_base = OpenLibrary( "dos.library", 50 );

	if ( ! dos_base )
	{
		return ( FALSE );
	}

	result = FALSE;

	dos = (PTR) GetInterface( dos_base, "main", 1, NULL );

	if ( ! dos )
	{
		goto close_dos;
	}

	data = AllocVecTags( RTL_FW_LIMIT + RTL_CFG_LIMIT + 1, TAG_END );

	if ( ! data )
	{
		goto drop_dos;
	}

	length = ReadFirmware( dos, RTL_FW_PATH, data, RTL_FW_LIMIT, FALSE );

	if ( length < 0 )
	{
		DebugPrintF( "Bluetooth USB: could not read " RTL_FW_PATH "\n" );
		goto free_data;
	}

	patch_size = FindPatch( data, length, rom, & offset );

	if ( ! patch_size )
	{
		DebugPrintF( "Bluetooth USB: invalid RTL8761BU firmware or no patch for ROM %lu\n", (U32) rom );
		goto free_data;
	}

	CopyMem( data + 8, data + offset + patch_size - 4, 4 );

	config = data + offset + patch_size;
	config_size = ReadFirmware( dos, RTL_CFG_PATH, config, RTL_CFG_LIMIT, TRUE );

	if ( ( config_size < 0 ) || ( ( config_size > 0 ) &&
		( ( config_size < 6 ) || ( ReadLE32( config ) != 0x8723ab55 ) ||
		  ( ReadLE16( config + 4 ) != config_size - 6 ) ) ) )
	{
		DebugPrintF( "Bluetooth USB: invalid " RTL_CFG_PATH "\n" );
		goto free_data;
	}

	if ( ( ! Download( usb, data + offset, patch_size + config_size ) ) ||
		( ! Command( usb, version_command, sizeof( version_command ), version, sizeof( version ) ) ) ||
		( ReadLE16( version + 6 ) == 0x8761 ) )
	{
		DebugPrintF( "Bluetooth USB: RTL8761BU firmware startup failed\n" );
		goto free_data;
	}

	DebugPrintF( "Bluetooth USB: RTL8761BU firmware ready, HCI $%04lx, LMP $%04lx\n",
		(U32) ReadLE16( version + 1 ), (U32) ReadLE16( version + 6 ) );

	result = TRUE;

free_data:
	FreeVec( data );

drop_dos:
	DropInterface( (PTR) dos );

close_dos:
	CloseLibrary( dos_base );

	return ( result );
}

// --
