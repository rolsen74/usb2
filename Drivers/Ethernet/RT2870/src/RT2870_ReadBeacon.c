
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#define RT2870_DMA_CONFIG		0x0208
#define RT2870_DMA_RX_ENABLE		0x00000004
#define RT2870_DMA_TX_ENABLE		0x00000001
#define RT2870_USB_DMA_CONFIG		0x02a0
#define RT2870_USB_DMA_RX_ENABLE	0x00400000
#define RT2870_USB_CLOCK		0x02a4
#define RT2870_USB_CLOCK_1US		30
#define RT2870_MAC_SYS_CTRL		0x1004
#define RT2870_MAC_RX_ENABLE		0x00000008
#define RT2870_RF_PIN_CONFIG		0x1328
#define RT2870_RX_PINS_2T2R		0x00050f00
#define RT2870_RF_BAND_CONFIG		0x132c
#define RT2870_RF_BAND_2GHZ		0x00000004
#define RT2870_RX_FILTER		0x1400
#define RT2870_RX_DROP_CRC_PHY		0x00000003
#define RT2870_AUTO_RESPONSE		0x1404
#define RT2870_RX_BUFFER_SIZE		4096
#define RT2870_RX_TIMEOUT		120000

// --

static const struct
{
	U16 address;
	U32 value;
} ReceiveRegisters[] =
{
	{ RT2870_AUTO_RESPONSE, 0 },
	{ RT2870_RF_BAND_CONFIG, RT2870_RF_BAND_2GHZ },
	{ RT2870_RF_PIN_CONFIG, RT2870_RX_PINS_2T2R },
	{ RT2870_RX_FILTER, RT2870_RX_DROP_CRC_PHY },
	{ RT2870_USB_DMA_CONFIG, RT2870_USB_DMA_RX_ENABLE },
	{ RT2870_MAC_SYS_CTRL, RT2870_MAC_RX_ENABLE }
};

// --

void
RT2870_StopBeacon( struct EthernetUnit *unit, S32 detached )
{
	struct USB2_EPResource *bulk;
	struct USB2_EPResource *control;
	struct USB2_IORequest *request;

	unit->unit_EAPOLLength = 0;
	unit->unit_AssocID = 0;
	unit->unit_SAE = FALSE;
	unit->unit_TKIP = FALSE;
	unit->unit_LastBeaconInterval = 0;
	unit->unit_PeerBeaconInterval = 0;
	unit->unit_Operational = FALSE;
	unit->unit_M4Sent = FALSE;
	unit->unit_TKIPRxReports = 0;
	unit->unit_TKIPRxFailures = 0;
	unit->unit_TKIPEAPOLReports = 0;
	unit->unit_TransmitEnabled = FALSE;
	unit->unit_DataOFDM = FALSE;
	unit->unit_DataSent = FALSE;
	unit->unit_DataReceived = FALSE;
	unit->unit_DHCPSent = 0;
	unit->unit_DHCPDelivered = 0;
	unit->unit_DHCPDropped = 0;

	RT2870_SetOnline( unit, FALSE );

	RT2870_ClearKeys( unit );

	memset( unit->unit_Peer, 0, sizeof( unit->unit_Peer ) );

	bulk = unit->unit_BeaconResource;

	if ( ! bulk )
	{
		return;
	}

	if ( unit->unit_ReceivePending )
	{
		request = bulk->IORequests[0];

		if ( ! CheckIO( (PTR) request ) )
		{
			AbortIO( (PTR) request );
		}

		WaitIO( (PTR) request );

		unit->unit_ReceivePending = FALSE;
	}

	control = unit->unit_USB_Register->Res_Control;

	if ( ( ! detached ) && ( unit->unit_BeaconEnabled ) )
	{
		RT2870_WriteRegister( control, RT2870_MAC_SYS_CTRL, 0 );

		if ( unit->unit_TimingActive )
		{
			RT2870_SetStationTiming( unit, 0 );
		}

		if ( unit->unit_PeerConfigured )
		{
			RT2870_SetStationPeer( unit, FALSE );
		}

		RT2870_WriteRegister( control, RT2870_USB_DMA_CONFIG, 0 );

		RT2870_WriteRegister( control, RT2870_DMA_CONFIG, unit->unit_BeaconDMA );

		RT2870_WriteRegister( control, RT2870_AUTO_RESPONSE, 0 );

		RT2870_WriteRegister( control, RT2870_RF_PIN_CONFIG, RT2870_RX_PINS_2T2R );
	}

	USB2_EPRes_Release( bulk );

	unit->unit_BeaconResource = NULL;
	unit->unit_BeaconEnabled = FALSE;
	unit->unit_TimingActive = FALSE;
	unit->unit_PeerConfigured = FALSE;

	USBINFO( "RT2870: channel %lu scan ended, %lu reads, %lu bytes, %lu timeouts, %lu beacons delivered",
		(U32) unit->unit_Channel, unit->unit_BeaconReads, unit->unit_BeaconBytes,
		unit->unit_BeaconTimeouts, unit->unit_BeaconsDelivered );
}

// --

S32
RT2870_StartReceive( struct EthernetUnit *unit )
{
	struct USB2_Register *reg;
	struct USB2_EPResource *bulk;
	struct USB2_EPResource *control;
	struct USB2_IORequest *request;

	U32 index;
	U32 clock;
	S32 error;

	if ( unit->unit_BeaconResource )
	{
		return ( 0 );
	}

	reg = unit->unit_USB_Register;
	control = reg->Res_Control;

	bulk = USB2_EPRes_ObtainTags( reg, USB2Tag_EPRes_InterfaceNr, 0,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In, USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_NrOfIORequest, 1, USB2Tag_EPRes_BufferSize, RT2870_RX_BUFFER_SIZE,
		USB2Tag_EPRes_AllowShortPackets, TRUE, USB2Tag_EPRes_TimeOut, RT2870_RX_TIMEOUT, TAG_END );

	if ( ! bulk )
	{
		return ( USB2Err_Stack_EndPointNotFound );
	}

	if ( unit->unit_RxHalted )
	{
		if ( ! USB2_EPRes_Destall( bulk ) )
		{
			USB2_EPRes_Release( bulk );

			return ( USB2Err_Host_Stall );
		}

		unit->unit_RxHalted = FALSE;
	}

	unit->unit_BeaconResource = bulk;
	unit->unit_BeaconReads = 0;
	unit->unit_BeaconBytes = 0;
	unit->unit_BeaconTimeouts = 0;
	unit->unit_BeaconsDelivered = 0;

	error = RT2870_ReadRegister( control, RT2870_USB_CLOCK, & clock );

	if ( ! error )
	{
		error = RT2870_WriteRegister( control, RT2870_USB_CLOCK, ( clock & ~0xff ) | RT2870_USB_CLOCK_1US );
	}

	if ( ! error )
	{
		error = RT2870_ReadRegister( control, RT2870_DMA_CONFIG, & unit->unit_BeaconDMA );
	}

	if ( ! error )
	{
		unit->unit_BeaconEnabled = TRUE;

		error = RT2870_WriteRegister( control, RT2870_DMA_CONFIG,
			( unit->unit_BeaconDMA & ~RT2870_DMA_TX_ENABLE ) | RT2870_DMA_RX_ENABLE );
	}

	for( index = 0; ( ! error ) && ( index < sizeof( ReceiveRegisters ) / sizeof( ReceiveRegisters[0] ) ); index++ )
	{
		error = RT2870_WriteRegister( control, ReceiveRegisters[index].address, ReceiveRegisters[index].value );
	}

	if ( error )
	{
		RT2870_StopBeacon( unit, error == USB2Err_Device_Detached );

		return ( error );
	}

	request = bulk->IORequests[0];
	request->io_Command = CMD_READ;
	request->io_Data = bulk->Buffers[0];
	request->io_Length = bulk->BufferSize;

	USBINFO( "RT2870: channel %lu scan started, bulk IN max packet %lu", (U32) unit->unit_Channel, bulk->EndPointMax );

	return ( 0 );
}

// --

S32
RT2870_ReadBeacon( struct EthernetUnit *unit )
{
	struct USB2_EPResource *bulk;
	struct USB2_IORequest *request;

	S32 error;

	error = RT2870_StartReceive( unit );

	if ( error )
	{
		return ( error );
	}

	bulk = unit->unit_BeaconResource;
	request = bulk->IORequests[0];

	if ( unit->unit_ReceivePending )
	{
		if ( ! CheckIO( (PTR) request ) )
		{
			return ( 0 );
		}

		error = WaitIO( (PTR) request );

		unit->unit_ReceivePending = FALSE;

		if ( error == USB2Err_Host_Stall )
		{
			unit->unit_RxHalted = ! USB2_EPRes_Destall( bulk );
		}
	}

	if ( error == USB2Err_Host_Timeout )
	{
		unit->unit_BeaconTimeouts++;
	}
	else if ( ( ! error ) && ( request->io_Actual ) )
	{
		if ( request->io_Actual > bulk->BufferSize )
		{
			error = USB2Err_Host_Overflow;
		}
		else
		{
			unit->unit_BeaconReads++;
			unit->unit_BeaconBytes += request->io_Actual;

			RT2870_ReceiveBeacon( unit, request->io_Data, request->io_Actual );
		}
	}

	if ( ( ( ! error ) || ( error == USB2Err_Host_Timeout ) ) && ( unit->unit_BeaconResource ) )
	{
		request->io_Actual = 0;
		unit->unit_ReceivePending = TRUE;

		SendIO( (PTR) request );

		return ( 0 );
	}

	return ( error );
}

// --
