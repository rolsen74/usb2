
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <proto/exec.h>
#include "SDRTest.h"

// --

#define RTL_WRITE		   0x0010
#define RTL_USB_BLOCK	   0x0100
#define RTL_SYS_BLOCK	   0x0200
#define RTL_USB_SYSCTL	   0x2000
#define RTL_SYS_DEMOD_CTL  0x3000
#define RTL_SYS_DEMOD_CTL1 0x300b

// --

S32
SDR_Control ( struct USB2_EPResource * control, U16 address, U16 index, U8 * data, U16 length )
{
	struct USB2_IORequest * request;
	struct USB2_SetupData * setup;

	S32 error;
	S32 write;

	request						  = control->IORequests[0];
	setup						  = control->SetupData;
	write						  = index & RTL_WRITE;
	setup->RequestType			  = REQTYPE_Vendor | ( write ? REQTYPE_Write : REQTYPE_Read );
	setup->RequestCode			  = 0;
	setup->Value				  = LE_SWAP16 ( address );
	setup->Index				  = LE_SWAP16 ( index );
	setup->Length				  = LE_SWAP16 ( length );
	request->io_Command			  = write ? CMD_WRITE : CMD_READ;
	request->io_SetupData		  = setup;
	request->io_SetupLength		  = sizeof ( *setup );
	request->io_Data			  = data;
	request->io_Length			  = length;
	request->io_AllowShortPackets = FALSE;

	error = DoIO ( (PTR)request );

	request->io_Data = NULL;

	if ( ( ! error ) && ( request->io_Actual != length ) )
	{
		error = USB2Err_Host_RuntPacket;
	}

	if ( error == USB2Err_Host_Stall )
	{
		USB2_EPRes_Destall ( control );
	}

	return ( error );
}

// --

S32
SDR_WriteRegister ( struct USB2_EPResource * control, U16 address, U16 index, U16 value, U16 length )
{
	U8	data[2];
	S32 error;

	if ( ( length != 1 ) && ( length != 2 ) )
	{
		return ( USB2Err_Stack_InvalidStructure );
	}

	data[0] = ( length == 2 ) ? value >> 8 : value;
	data[1] = value;

	error = SDR_Control ( control, address, index | RTL_WRITE, data, length );

	if ( error )
	{
		Printf ( "RTL2832U register $%04lx index $%04lx write failed (USB error %ld).\n", (U32)address, (U32)index, error );
	}

	return ( error );
}

// --

S32
SDR_WriteDemod ( struct USB2_EPResource * control, U8 page, U8 reg, U16 value, U16 length )
{
	U8	readback;
	S32 error;

	error = SDR_WriteRegister ( control, ( (U16)reg << 8 ) | 0x20, page, value, length );

	if ( ! error )
	{
		// Flush the demodulator write through its USB register interface.
		error = SDR_Control ( control, 0x0120, 0x000a, &readback, 1 );

		if ( error )
		{
			Printf ( "RTL2832U demodulator readback failed (USB error %ld).\n", error );
		}
	}

	return ( error );
}

// --

S32
SDR_SetI2CGate ( struct USB2_EPResource * control, S32 enabled )
{
	return ( SDR_WriteDemod ( control, 1, 0x01, enabled ? 0x18 : 0x10, 1 ) );
}

// --

S32
SDR_PowerOn ( struct USB2_EPResource * control )
{
	S32 error;

	if ( ( ! control ) || ( ! control->SetupData ) || ( ! control->IORequests ) || ( ! control->IORequests[0] ) )
	{
		Printf ( "No USB control request available for the RTL2832U.\n" );

		return ( USB2Err_Stack_InvalidStructure );
	}

	error = SDR_WriteRegister ( control, RTL_USB_SYSCTL, RTL_USB_BLOCK, 0x09, 1 );

	if ( error )
	{
		return ( error );
	}

	error = SDR_WriteRegister ( control, RTL_SYS_DEMOD_CTL1, RTL_SYS_BLOCK, 0x22, 1 );

	if ( ! error )
	{
		error = SDR_WriteRegister ( control, RTL_SYS_DEMOD_CTL, RTL_SYS_BLOCK, 0xe8, 1 );
	}

	if ( error )
	{
		SDR_PowerOff ( control );
	}

	return ( error );
}

// --

S32
SDR_PowerOff ( struct USB2_EPResource * control )
{
	return ( SDR_WriteRegister ( control, RTL_SYS_DEMOD_CTL, RTL_SYS_BLOCK, 0x20, 1 ) );
}

// --
