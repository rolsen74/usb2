
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"
#include "Registers.h"

// --

#define H2C_SIZE 32
#define H2C_QUEUE 0x13
#define H2C_GENERAL_INFO 0x0d
#define H2C_IQK 0x0e
#define H2C_PHY_INFO 0x11
#define RF_IQK_STATUS 0x2820
#define RF_IQK_DONE 0x000abcde

// --

static S32
SendCalibrationCommand( struct EthernetUnit *unit, U8 command, U8 length, U8 sequence, U32 data )
{
	struct USB2_EPResource *resource;
	struct USB2_IORequest *ioreq;

	U8 *descriptor;
	U8 *packet;

	resource = unit->unit_Bulk_Out[RTL8821CU_TX_MANAGEMENT];
	ioreq = resource->IORequests[0];
	descriptor = resource->Buffers[0];
	packet = descriptor + RTL8821CU_TX_DESC_SIZE;

	memset( descriptor, 0, RTL8821CU_TX_DESC_SIZE + H2C_SIZE );

	descriptor[0] = H2C_SIZE;
	descriptor[5] = H2C_QUEUE;
	packet[0] = 1;
	packet[1] = 0xff;
	packet[2] = command;
	packet[4] = length;
	packet[6] = sequence;
	packet[8] = data;
	packet[9] = data >> 8;
	packet[10] = data >> 16;
	packet[11] = data >> 24;

	RTL8821CU_ChecksumTX( descriptor );

	ioreq->io_Command = CMD_WRITE;
	ioreq->io_Data = descriptor;
	ioreq->io_Length = RTL8821CU_TX_DESC_SIZE + H2C_SIZE;
	ioreq->io_TimeOut = 500000;

	return ( ( ! DoIO( (PTR) ioreq ) ) && ( ioreq->io_Actual == ioreq->io_Length ) );
}

// --

S32
RTL8821CU_CalibrateRF( struct EthernetUnit *unit )
{
	struct USB2_EPResource *control;

	U32 count;
	U32 status;
	U32 phy;

	control = unit->unit_USB_Register->Res_Control;
	phy = 0x11000400U | ( (U32) unit->unit_Cut << 16 ) | ( unit->unit_RFOption & 0x1f );

	if ( ( ! SendCalibrationCommand( unit, H2C_GENERAL_INFO, 12, 0, RTL8821CU_FW_TX_OFFSET_PAGES << 16 ) ) ||
		( ! SendCalibrationCommand( unit, H2C_PHY_INFO, 16, 1, phy ) ) ||
		( ! RTL8821CU_UpdateRF( control, 0x08, 0x000fffff, 0 ) ) ||
		( ! SendCalibrationCommand( unit, H2C_IQK, 9, 2, 0 ) ) )
	{
		return ( FALSE );
	}

	for( count = 0; count < 300; count++ )
	{
		if ( ! RTL8821CU_ReadReg( control, RF_IQK_STATUS, 4, & status ) )
		{
			return ( FALSE );
		}

		if ( ( status & 0x000fffff ) == RF_IQK_DONE )
		{
			DebugPrintF( "RTL8821CU: firmware RF calibration complete\n" );

			return ( RTL8821CU_UpdateRF( control, 0x08, 0x000fffff, 0 ) );
		}

		Delay( 1 );
	}

	DebugPrintF( "RTL8821CU: firmware RF calibration timed out\n" );

	RTL8821CU_UpdateRF( control, 0x08, 0x000fffff, 0 );

	return ( FALSE );
}

// --
