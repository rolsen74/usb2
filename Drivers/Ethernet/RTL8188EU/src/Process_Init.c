
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
Process_Init( struct EthernetUnit *unit, struct USB2_DriverMessage *msg )
{
	unit->unit_USB_Interface = msg->Interface;

	unit->unit_USB_Register = USB2_Reg_RegisterTags(
		USB2Tag_Reg_DriverMessage, msg,
		USB2Tag_Reg_Title, "RTL8188EU",
		TAG_END );

	if ( ! unit->unit_USB_Register )
	{
		USBERROR( "RTL8188EU: USB interface registration failed" );

		return ( FALSE );
	}

	if ( ! RTL8188EU_Probe( unit->unit_USB_Register->Res_Control ) )
	{
		return ( FALSE );
	}

	unit->unit_StationAddressValid = RTL8188EU_ReadStationAddress( unit );

	unit->unit_Bulk_Rx_Resource = USB2_EPRes_ObtainTags( unit->unit_USB_Register,
		USB2Tag_EPRes_InterfaceNr, unit->unit_USB_Interface->IFC_Num,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_BufferSize, 4096,
		USB2Tag_EPRes_NrOfIORequest, WIRELESS_RX_REQUEST_COUNT,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		TAG_END );

	if ( ! unit->unit_Bulk_Rx_Resource )
	{
		USBERROR( "RTL8188EU: could not obtain the bulk IN endpoint" );

		return ( FALSE );
	}

	unit->unit_Bulk_Tx_Resource = USB2_EPRes_ObtainTags( unit->unit_USB_Register,
		USB2Tag_EPRes_InterfaceNr, unit->unit_USB_Interface->IFC_Num,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_Out,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_BufferSize, 2048,
		USB2Tag_EPRes_NrOfIORequest, 1,
		TAG_END );

	if ( ! unit->unit_Bulk_Tx_Resource )
	{
		USBERROR( "RTL8188EU: could not obtain the bulk OUT endpoint" );

		return ( FALSE );
	}

	return ( RTL8188EU_LoadFirmware( unit ) );
}

// --
