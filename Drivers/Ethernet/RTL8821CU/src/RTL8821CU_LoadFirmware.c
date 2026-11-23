
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"

// --

#define FIRMWARE_PATH "DEVS:Firmware/rtw8821c_fw.bin"
#define FIRMWARE_MAX_SIZE ( 256 * 1024 )
#define FW_IMEM_BASE 0x00030000
#define FW_EMEM_BASE 0x00100000
#define FW_DMEM_LIMIT 0x00300000

// --

static U32
ReadLE32( const U8 *bytes )
{
	return ( (U32) bytes[0] | ( (U32) bytes[1] << 8 ) |
		( (U32) bytes[2] << 16 ) | ( (U32) bytes[3] << 24 ) );
}

// --

static U32
ReadSections( struct FirmwareSection *sections, const U8 *image, U32 length )
{
	static const U8 SizeOffset[FW_SECTION_COUNT] = { 0x24, 0x30, 0x34 };
	static const U8 AddressOffset[FW_SECTION_COUNT] = { 0x20, 0x3c, 0x38 };
	static const U32 Start[FW_SECTION_COUNT] = { FW_DMEM_BASE, FW_IMEM_BASE, FW_EMEM_BASE };
	static const U32 End[FW_SECTION_COUNT] = { FW_DMEM_LIMIT, FW_EMEM_BASE, FW_DMEM_BASE };

	U32 count;
	U32 offset;
	U32 index;
	U32 size;
	U32 address;

	if ( length < FW_HEADER_SIZE )
	{
		return ( 0 );
	}

	count = 2;
	offset = FW_HEADER_SIZE;

	if ( ( image[0x18] & 0x10 ) && ( ReadLE32( image + 0x34 ) ) )
	{
		count++;
	}

	for( index = 0; index < count; index++ )
	{
		size = ReadLE32( image + SizeOffset[index] );

		if ( ( ! size ) || ( length - offset < FW_CHECKSUM_SIZE ) ||
			( size > length - offset - FW_CHECKSUM_SIZE ) )
		{
			return ( 0 );
		}

		size += FW_CHECKSUM_SIZE;

		address = ReadLE32( image + AddressOffset[index] );

		address &= 0x7fffffff;

		// Only allow DDMA transfers into WLAN CPU memory.
		if ( ( address < Start[index] ) || ( address >= End[index] ) || ( size > End[index] - address ) )
		{
			return ( 0 );
		}

		sections[index].Data = image + offset;
		sections[index].Address = address;
		sections[index].Length = size;
		offset += size;
	}

	return ( ( offset == length ) ? count : 0 );
}

// --

static U32
ReadImage( BPTR file, U8 *image )
{
	S32 actual;
	U32 offset;

	offset = 0;

	while( offset <= FIRMWARE_MAX_SIZE )
	{
		actual = Read( file, image + offset, FIRMWARE_MAX_SIZE + 1 - offset );

		if ( actual <= 0 )
		{
			return ( ( actual == 0 ) ? offset : 0 );
		}

		offset += actual;
	}

	return ( 0 );
}

// --

S32
RTL8821CU_LoadFirmware( struct EthernetUnit *unit )
{
	struct FirmwareSection sections[FW_SECTION_COUNT];
	struct USB2_EPResource *control;

	U8 *image;
	BPTR file;
	U32 length;
	U32 count;
	S32 ready;

	file = Open( FIRMWARE_PATH, MODE_OLDFILE );

	if ( ! file )
	{
		DebugPrintF( "RTL8821CU: could not open " FIRMWARE_PATH "\n" );

		return ( FALSE );
	}

	image = AllocVecTags( FIRMWARE_MAX_SIZE + 1, TAG_END );

	if ( ! image )
	{
		Close( file );

		return ( FALSE );
	}

	length = ReadImage( file, image );

	Close( file );

	count = ReadSections( sections, image, length );

	if ( ! count )
	{
		DebugPrintF( "RTL8821CU: invalid or unreadable firmware image\n" );

		FreeVec( image );

		return ( FALSE );
	}

	control = unit->unit_USB_Register->Res_Control;

	ready = RTL8821CU_PowerOn( control );

	if ( ready )
	{
		ready = RTL8821CU_DownloadFirmware( unit, sections, count );
	}

	if ( ready )
	{
		DebugPrintF( "RTL8821CU: WLAN firmware %lu.%lu.%lu ready\n",
			( (U32) image[5] << 8 ) | image[4], (U32) image[6], (U32) image[7] );
	}
	else
	{
		DebugPrintF( "RTL8821CU: WLAN firmware startup failed\n" );

		RTL8821CU_PowerOff( control );
	}

	FreeVec( image );

	return ( ready );
}

// --
