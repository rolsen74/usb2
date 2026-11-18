
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
	U32 signature;
	U32 payload;
	U32 endpoints;

	control = unit->unit_USB_Register->Res_Control;

	file = Open( RTL8188EU_FIRMWARE_PATH, MODE_OLDFILE );

	if ( ! file )
	{
		// Keep USB probing available when no firmware has been installed.
		return ( IoErr() == ERROR_OBJECT_NOT_FOUND || IoErr() == ERROR_DIR_NOT_FOUND );
	}

	image = NULL;
	succeeded = FALSE;

	length = GetFileSize( file );

	if ( ( length <= RTL8188EU_FW_HEADER_SIZE ) ||
		( length > RTL8188EU_FW_HEADER_SIZE + RTL8188EU_FW_MAX_SIZE ) )
	{
		goto done;
	}

	image = AllocVecTags( (U32) length, TAG_END );

	if ( ! image )
	{
		goto done;
	}

	if ( Read( file, image, (S32) length ) != length )
	{
		goto done;
	}

	signature = image[0] | ( (U32) image[1] << 8 );
	payload = image[12] | ( (U32) image[13] << 8 );

	if ( ( signature >> 4 != 0x88e ) || ( payload != length - RTL8188EU_FW_HEADER_SIZE ) )
	{
		goto done;
	}

	endpoints = RTL8188EU_CountTXEndpoints( unit );

	if ( ( endpoints ) && ( unit->unit_StationAddressValid ) &&
		( RTL8188EU_PowerOn( control ) ) && ( RTL8188EU_InitDMA( control, endpoints ) ) &&
		( RTL8188EU_UploadFirmware( control, image + RTL8188EU_FW_HEADER_SIZE, payload ) ) )
	{
		unit->unit_FirmwareRunning = TRUE;

		succeeded = RTL8188EU_StartRadio( unit );

		unit->unit_RadioReady = succeeded;
	}

done:

	if ( image )
	{
		FreeVec( image );
	}

	Close( file );

	if ( ! succeeded )
	{
		DebugPrintF( "RTL8188EU: could not initialize firmware from %s\n", RTL8188EU_FIRMWARE_PATH );
	}

	return ( succeeded );
}

// --
