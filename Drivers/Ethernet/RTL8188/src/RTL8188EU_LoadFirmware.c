
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"
#include "Radio.h"

// --

S32
RTL8188EU_LoadFirmware( struct EthernetUnit *unit )
{
	struct USB2_EPResource *control;

	BPTR file;
	U8 *image;
	S64 length;
	S32 succeeded;
	S32 error;
	U32 signature;
	U32 payload;
	U32 endpoints;

	control = unit->unit_USB_Register->Res_Control;

	file = Open( RTL8188EU_FIRMWARE_PATH, MODE_OLDFILE );

	if ( ! file )
	{
		error = IoErr();

		// Keep USB probing available when no firmware has been installed.
		if ( ( error == ERROR_OBJECT_NOT_FOUND ) || ( error == ERROR_DIR_NOT_FOUND ) )
		{
			USBERROR( "RTL8188EU: firmware missing at %s, unit will stay offline", RTL8188EU_FIRMWARE_PATH );

			return ( TRUE );
		}

		USBERROR( "RTL8188EU: could not open firmware %s, DOS error %ld", RTL8188EU_FIRMWARE_PATH, error );

		return ( FALSE );
	}

	image = NULL;
	succeeded = FALSE;

	length = GetFileSize( file );

	if ( ( length <= RTL8188EU_FW_HEADER_SIZE ) ||
		( length > RTL8188EU_FW_HEADER_SIZE + RTL8188EU_FW_MAX_SIZE ) )
	{
		USBERROR( "RTL8188EU: invalid firmware file size" );

		goto done;
	}

	image = AllocVecTags( (U32) length, TAG_END );

	if ( ! image )
	{
		USBERROR( "RTL8188EU: could not allocate %lu bytes for firmware", (U32) length );

		goto done;
	}

	if ( Read( file, image, (S32) length ) != length )
	{
		USBERROR( "RTL8188EU: could not read the complete firmware file" );

		goto done;
	}

	signature = image[0] | ( (U32) image[1] << 8 );
	payload = image[12] | ( (U32) image[13] << 8 );

	if ( ( signature >> 4 != 0x88e ) || ( payload != length - RTL8188EU_FW_HEADER_SIZE ) )
	{
		USBERROR( "RTL8188EU: invalid firmware header, signature 0x%04lx, payload %lu, file size %lu",
			signature, payload, (U32) length );

		goto done;
	}

	endpoints = RTL8188EU_CountTXEndpoints( unit );

	USBINFO( "RTL8188EU: firmware payload %lu bytes, TX endpoints %lu, valid station address %lu",
		payload, endpoints, (U32) unit->unit_StationAddressValid );

	if ( ( ! endpoints ) || ( ! unit->unit_StationAddressValid ) )
	{
		goto done;
	}

	unit->unit_BringupStarted = TRUE;

	if ( ! RTL8188EU_PowerOn( control ) )
	{
		USBERROR( "RTL8188EU: chip power-on failed" );

		goto done;
	}

	if ( ! RTL8188EU_InitDMA( control, endpoints ) )
	{
		USBERROR( "RTL8188EU: DMA initialization failed" );

		goto done;
	}

	if ( ! RTL8188EU_UploadFirmware( control, image + RTL8188EU_FW_HEADER_SIZE, payload ) )
	{
		USBERROR( "RTL8188EU: firmware upload or startup handshake failed" );

		goto done;
	}

	unit->unit_FirmwareRunning = TRUE;

	succeeded = RTL8188EU_StartRadio( unit );

	unit->unit_RadioReady = succeeded;

	if ( succeeded )
	{
		USBINFO( "RTL8188EU: firmware running, radio ready on channel %lu, TX ready %lu",
			(U32) unit->unit_Channel, (U32) unit->unit_TxReady );
	}
	else
	{
		USBERROR( "RTL8188EU: firmware started but radio initialization failed" );
	}

done:

	if ( image )
	{
		FreeVec( image );
	}

	Close( file );

	if ( ! succeeded )
	{
		USBERROR( "RTL8188EU: could not initialize adapter using %s", RTL8188EU_FIRMWARE_PATH );
	}

	return ( succeeded );
}

// --
