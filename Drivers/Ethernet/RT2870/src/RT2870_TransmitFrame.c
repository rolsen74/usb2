
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define RT2870_TXINFO_SIZE		4
#define RT2870_TXWI_SIZE		16
#define RT2870_TX_END_PAD		4
#define RT2870_TX_BUFFER_SIZE		( RT2870_TXINFO_SIZE + RT2870_TXWI_SIZE + RT2870_MAX_FRAME + RT2870_TX_END_PAD )
#define RT2870_TXINFO_WIV		0x01
#define RT2870_TXINFO_EDCA		0x04
#define RT2870_TXWI_BACKOFF		0x03
#define RT2870_TXWI_ACK			0x01
#define RT2870_TXWI_HW_SEQUENCE		0x02
#define RT2870_TXWI_OFDM_6MBPS		0x40	// Upper byte of the PHY field
#define RT2870_CCK_ACK_DURATION		314
#define RT2870_OFDM_ACK_DURATION	60
#define RT2870_TXWI_NO_WCID		0xff

#define RT2870_DMA_CONFIG		0x0208
#define RT2870_DMA_RX_TX_ENABLE	0x00000005
#define RT2870_USB_DMA_CONFIG		0x02a0
#define RT2870_USB_DMA_RX_TX_ENABLE	0x00c00000
#define RT2870_MAC_SYS_CTRL		0x1004
#define RT2870_MAC_RX_TX_ENABLE	0x0000000c
#define RT2870_MAC_ADDR			0x1008
#define RT2870_MAC_BSSID		0x1010
#define RT2870_TX_PIN_CONFIG		0x1328
#define RT2870_TX_RX_PINS_2T2R		0x00050f0a
#define RT2870_AUTO_RESPONSE		0x1404
#define RT2870_TX_STATUS_FIFO		0x1718
#define RT2870_TX_STATUS_VALID		0x00000001
#define RT2870_TX_STATUS_PID		0x0000001e
#define RT2870_TX_STATUS_SUCCESS	0x00000020
#define RT2870_TX_STATUS_ACK_REQUIRED	0x00000080

// --

// RT2870 USB MAC defaults for legacy management transmission from OpenBSD run(4).
static const struct
{
	U16 address;
	U32 value;
} TransmitRegisters[] =
{
	{ 0x6804, 0 }, // Receive CCMP frames with their IV and MIC
	{ 0x7000, 0 }, // Shared key cipher modes
	{ 0x0214, 0x00002273 }, // WMM AIFSN
	{ 0x0218, 0x00002344 }, // WMM CWMIN
	{ 0x021c, 0x000034aa }, // WMM CWMAX
	{ 0x0408, 0x00f40006 }, // Packet buffer configuration
	{ 0x040c, 0x1f3fbf9f }, // Packet buffer queue limits
	{ 0x1100, 0x33a41010 }, // Interframe spacing
	{ 0x1104, 0x00000209 }, // Backoff slot
	{ 0x1204, 0x00000003 }, // Power pins
	{ 0x1330, 0x00000000 }, // TX switch configuration 0
	{ 0x1334, 0x00080606 }, // TX switch configuration 1
	{ 0x1348, 0x000a2090 }, // TX timeout
	{ 0x134c, 0x47d01f0f }, // TX retry
	{ 0x1350, 0x00001020 }, // TX link
	{ 0x1408, 0x0000015f }, // Legacy basic rates
	{ RT2870_TX_PIN_CONFIG, RT2870_TX_RX_PINS_2T2R },
	{ RT2870_AUTO_RESPONSE, 1 }
};

// --

static S32
WriteAddress( struct USB2_EPResource *control, U16 reg, const U8 *addr, U32 high )
{
	U32 low;
	S32 error;

	low = addr[0] | ( (U32) addr[1] << 8 ) | ( (U32) addr[2] << 16 ) | ( (U32) addr[3] << 24 );
	high |= addr[4] | ( (U32) addr[5] << 8 );

	error = RT2870_WriteRegister( control, reg, low );

	if ( ! error )
	{
		error = RT2870_WriteRegister( control, reg + 4, high );
	}

	return ( error );
}

// --

static S32
EnableTransmit( struct EthernetUnit *unit, const U8 *bssid )
{
	struct USB2_EPResource *control;

	U32 index;
	S32 error;

	control = unit->unit_USB_Register->Res_Control;

	error = WriteAddress( control, RT2870_MAC_ADDR, unit->unit_StationAddress, 0x00ff0000 );

	if ( ! error )
	{
		error = WriteAddress( control, RT2870_MAC_BSSID, bssid, 0 );
	}

	for( index = 0; ( ! error ) && ( index < sizeof( TransmitRegisters ) / sizeof( TransmitRegisters[0] ) ); index++ )
	{
		error = RT2870_WriteRegister( control, TransmitRegisters[index].address, TransmitRegisters[index].value );
	}

	if ( ! error )
	{
		error = RT2870_WriteRegister( control, RT2870_DMA_CONFIG, unit->unit_BeaconDMA | RT2870_DMA_RX_TX_ENABLE );
	}

	if ( ! error )
	{
		error = RT2870_WriteRegister( control, RT2870_USB_DMA_CONFIG, RT2870_USB_DMA_RX_TX_ENABLE );
	}

	if ( ! error )
	{
		error = RT2870_WriteRegister( control, RT2870_MAC_SYS_CTRL, RT2870_MAC_RX_TX_ENABLE );
	}

	return ( error );
}

// --

static void
ReportTransmitStatus( struct USB2_EPResource *control, U32 pid, const char *kind )
{
	U32 status;
	U32 attempt;
	S32 error;

	for( attempt = 0; attempt < 5; attempt++ )
	{
		error = RT2870_ReadRegister( control, RT2870_TX_STATUS_FIFO, & status );

		if ( error )
		{
			return;
		}

		if ( ( status & RT2870_TX_STATUS_VALID ) && ( ( status & RT2870_TX_STATUS_PID ) == ( pid << 1 ) ) )
		{
			USBINFO( "RT2870: %s TX status $%08lx, ACK required %ld, success %ld",
				kind, status, (S32) ( ( status & RT2870_TX_STATUS_ACK_REQUIRED ) != 0 ),
				(S32) ( ( status & RT2870_TX_STATUS_SUCCESS ) != 0 ) );

			return;
		}

		Delay( 1 );
	}

	USBINFO( "RT2870: no %s TX status for packet ID %lu", kind, pid );
}

// --

S32
RT2870_TransmitFrame( struct EthernetUnit *unit, const U8 *frame, U32 frameLength, const U8 *bssid, S32 data, const char *report )
{
	struct USB2_EPResource *bulk;
	struct USB2_IORequest *request;

	U8 *buffer;
	U32 pktLength;
	U32 transferLength;
	U32 actual;
	U32 pid;
	U16 duration;
	S32 associated;
	S32 ofdm;
	S32 error;

	actual = 0;
	associated = data && unit->unit_Operational && unit->unit_PeerConfigured;
	ofdm = associated && unit->unit_DataOFDM;
	duration = ( ( frameLength >= 24 ) && ( frame[4] & 1 ) ) ? 0 :
		( ofdm ? RT2870_OFDM_ACK_DURATION : RT2870_CCK_ACK_DURATION );

	if ( ( frameLength < 24 ) || ( frameLength > RT2870_MAX_FRAME ) )
	{
		return ( USB2Err_Host_Overflow );
	}

	bulk = unit->unit_BulkOut;

	if ( ! bulk )
	{
		// Keep the DMA buffer and request for the lifetime of the unit.
		bulk = USB2_EPRes_ObtainTags( unit->unit_USB_Register,
			USB2Tag_EPRes_InterfaceNr, 0, USB2Tag_EPRes_EPDirection, EPADR_Dir_Out,
			USB2Tag_EPRes_EPType, EPATT_Type_Bulk, USB2Tag_EPRes_NrOfIORequest, 1,
			USB2Tag_EPRes_BufferSize, RT2870_TX_BUFFER_SIZE, USB2Tag_EPRes_TimeOut, 300000,
			USB2Tag_EPRes_AddZeroPacket, TRUE, TAG_END );

		if ( ! bulk )
		{
			return ( USB2Err_Stack_EndPointNotFound );
		}

		unit->unit_BulkOut = bulk;
	}

	pktLength = ( RT2870_TXWI_SIZE + frameLength + 3 ) & ~3U;
	transferLength = RT2870_TXINFO_SIZE + pktLength + RT2870_TX_END_PAD;
	pid = ( unit->unit_ManagementPID % 3 + 1 ) * 4;
	unit->unit_ManagementPID = pid >> 2;

	if ( unit->unit_TxHalted )
	{
		if ( ! USB2_EPRes_Destall( bulk ) )
		{
			return ( USB2Err_Host_Stall );
		}

		unit->unit_TxHalted = FALSE;
	}

	error = RT2870_StartReceive( unit );

	if ( ( ! error ) && ( ( ! unit->unit_TransmitEnabled ) || ( ! data ) ) )
	{
		error = EnableTransmit( unit, bssid );

		unit->unit_TransmitEnabled = ! error;
	}

	if ( ! error )
	{
		buffer = bulk->Buffers[0];

		memset( buffer, 0, transferLength );

		buffer[0] = (U8) pktLength;
		buffer[1] = pktLength >> 8;
		buffer[3] = RT2870_TXINFO_EDCA | ( data ? RT2870_TXINFO_WIV : 0 );
		buffer[5] = RT2870_TXWI_BACKOFF;
		buffer[7] = ofdm ? RT2870_TXWI_OFDM_6MBPS : 0;
		buffer[8] = RT2870_TXWI_HW_SEQUENCE | ( ( frame[4] & 1 ) ? 0 : RT2870_TXWI_ACK );
		buffer[9] = associated ? RT2870_AP_WCID : RT2870_TXWI_NO_WCID;
		buffer[10] = (U8) frameLength;
		buffer[11] = ( frameLength >> 8 ) | ( pid << 4 );

		memcpy( buffer + RT2870_TXINFO_SIZE + RT2870_TXWI_SIZE, frame, frameLength );

		buffer[RT2870_TXINFO_SIZE + RT2870_TXWI_SIZE + 2] = duration;
		buffer[RT2870_TXINFO_SIZE + RT2870_TXWI_SIZE + 3] = duration >> 8;

		request = bulk->IORequests[0];
		request->io_Command = CMD_WRITE;
		request->io_Data = buffer;
		request->io_Length = transferLength;
		request->io_Actual = 0;

		error = DoIO( (PTR) request );

		actual = request->io_Actual;

		if ( error == USB2Err_Host_Stall )
		{
			unit->unit_TxHalted = ! USB2_EPRes_Destall( bulk );
		}

		if ( ( ! error ) && ( actual != transferLength ) )
		{
			error = USB2Err_Host_RuntPacket;
		}
	}

	if ( ( report ) || ( error ) )
	{
		USBINFO( "RT2870: %s TX, USB error %ld, %lu/%lu bytes", report ? report : "data", error, actual, transferLength );
	}

	if ( ( ! error ) && ( report ) )
	{
		ReportTransmitStatus( unit->unit_USB_Register->Res_Control, pid, report );
	}
	else if ( error )
	{
		RT2870_DropLink( unit, error == USB2Err_Device_Detached );
	}

	return ( error );
}

// --
