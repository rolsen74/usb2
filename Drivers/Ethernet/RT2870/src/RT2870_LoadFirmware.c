
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define RT2870_FW_PATH			"DEVS:Firmware/rt2870.bin"
#define RT2870_FW_BANK_SIZE		4096
#define RT2870_FW_BASE			0x3000

#define RT2870_DEVICE_MODE		0x01
#define RT2870_MODE_START_FIRMWARE	0x08

#define RT2870_DMA_CONFIG		0x0208
#define RT2870_DMA_ENABLE_BUSY		0x0000000f
#define RT2870_DMA_TX_WRITEBACK		0x00000040
#define RT2870_PBF_SYS_CTRL		0x0400
#define RT2870_PBF_READY		0x00000080
#define RT2870_AUTOWAKE_CONFIG		0x1208
#define RT2870_MCU_COMMAND_ID		0x7014
#define RT2870_MCU_COMMAND_STATUS	0x701c

// --

static S32
ValidFirmwareBank( const U8 *image )
{
	U32 offset;
	U32 bit;
	U16 crc;

	crc = 0xffff;

	// Reflected CRC-CCITT. The final two bytes store the CRC low byte first.
	for( offset = 0; offset < RT2870_FW_BANK_SIZE - 2; offset++ )
	{
		crc ^= image[offset];

		for( bit = 0; bit < 8; bit++ )
		{
			crc = ( crc >> 1 ) ^ ( ( crc & 1 ) ? 0x8408 : 0 );
		}
	}

	return ( ( image[offset] == (U8) crc ) && ( image[offset + 1] == ( crc >> 8 ) ) );
}

// --

static S32
UploadFirmware( struct USB2_EPResource *control )
{
	BPTR file;
	U8 *image;
	S32 length;
	S32 transferred;
	S32 error;
	U32 offset;
	U16 word;

	image = NULL;
	error = FALSE;

	file = Open( RT2870_FW_PATH, MODE_OLDFILE );

	if ( ! file )
	{
		USBERROR( "RT2870: could not open %s", RT2870_FW_PATH );

		return ( FALSE );
	}

	image = AllocVecTags( RT2870_FW_BANK_SIZE, TAG_END );

	if ( ! image )
	{
		USBERROR( "RT2870: could not allocate firmware buffer" );
		goto bailout;
	}

	// RT2860, RT2872 and RT3070 use the first bank of rt2870.bin.
	length = Read( file, image, RT2870_FW_BANK_SIZE );

	if ( ( length != RT2870_FW_BANK_SIZE ) || ( ! ValidFirmwareBank( image ) ) )
	{
		USBERROR( "RT2870: invalid firmware bank in %s", RT2870_FW_PATH );
		goto bailout;
	}

	// RT2860 uses word writes because the region-write command can be unstable.
	for( offset = 0; offset < RT2870_FW_BANK_SIZE; offset += 2 )
	{
		word = image[offset] | ( (U16) image[offset + 1] << 8 );

		transferred = RT2870_WriteWord( control, RT2870_FW_BASE + offset, word );

		if ( transferred )
		{
			USBERROR( "RT2870: firmware upload failed at offset $%04lx, USB error %ld", offset, transferred );
			goto bailout;
		}
	}

	USBINFO( "RT2870: uploaded %lu bytes of MCU firmware", (U32) RT2870_FW_BANK_SIZE );

	error = TRUE;

bailout:

	FreeVec( image );

	Close( file );

	return ( error );
}

// --

S32
RT2870_LoadFirmware( struct USB2_EPResource *control, U32 chip )
{
	U32 value;
	U32 attempt;
	S32 error;

	if ( ( chip != 0x2860 ) && ( chip != 0x2872 ) && ( chip != 0x3070 ) )
	{
		USBERROR( "RT2870: firmware startup unsupported for MAC/BBP $%04lx", chip );

		return ( FALSE );
	}

	// Keep data DMA disabled during MCU startup.
	error = RT2870_WriteRegister( control, RT2870_AUTOWAKE_CONFIG, 0 );

	if ( ! error )
	{
		error = RT2870_ReadRegister( control, RT2870_DMA_CONFIG, & value );
	}

	if ( ! error )
	{
		value = ( value & ~RT2870_DMA_ENABLE_BUSY ) | RT2870_DMA_TX_WRITEBACK;

		error = RT2870_WriteRegister( control, RT2870_DMA_CONFIG, value );
	}

	if ( error )
	{
		USBERROR( "RT2870: firmware startup setup failed, USB error %ld", error );

		return ( FALSE );
	}

	if ( ! UploadFirmware( control ) )
	{
		return ( FALSE );
	}

	error = RT2870_WriteRegister( control, RT2870_MCU_COMMAND_ID, 0xffffffff );

	if ( ! error )
	{
		error = RT2870_WriteRegister( control, RT2870_MCU_COMMAND_STATUS, 0xffffffff );
	}

	if ( ! error )
	{
		error = RT2870_Control( control, REQTYPE_Write, RT2870_DEVICE_MODE, RT2870_MODE_START_FIRMWARE,
			0, NULL, 0 );
	}

	if ( error )
	{
		USBERROR( "RT2870: could not start MCU firmware, USB error %ld", error );

		return ( FALSE );
	}

	for( attempt = 0; attempt < 50; attempt++ )
	{
		Delay( 1 );

		error = RT2870_ReadRegister( control, RT2870_PBF_SYS_CTRL, & value );

		if ( error )
		{
			break;
		}

		if ( ( value != 0xffffffff ) && ( value & RT2870_PBF_READY ) )
		{
			USBINFO( "RT2870: MCU firmware ready, PBF_SYS_CTRL $%08lx", value );

			return ( TRUE );
		}
	}

	if ( error )
	{
		USBERROR( "RT2870: MCU firmware readiness check failed, USB error %ld", error );
	}
	else
	{
		USBERROR( "RT2870: MCU firmware ready bit timed out, PBF_SYS_CTRL $%08lx", value );
	}

	return ( FALSE );
}

// --
