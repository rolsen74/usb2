
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"

// --

static const U8 BulkOutEndpoints[RTL8821CU_TX_PIPES] = { 0x05, 0x06, 0x08 };

// --

S32
Process_Setup_Device( struct EthernetUnit *unit, struct USB2_DriverMessage *msg )
{
	struct USB2IFace *usb2;

	U32 index;

	usb2 = msg->IUSB2;

	unit->unit_USB_Register = usb2->USB2_Reg_RegisterTags(
		USB2Tag_Reg_DriverMessage, msg,
		USB2Tag_Reg_Title, "RTL8821CU Wi-Fi",
		TAG_END );

	if ( ! unit->unit_USB_Register )
	{
		return ( FALSE );
	}

	unit->unit_Bulk_In = usb2->USB2_EPRes_ObtainTags( unit->unit_USB_Register,
		USB2Tag_EPRes_InterfaceNr, RTL8821CU_INTERFACE,
		USB2Tag_EPRes_EndPointNr, 0x84,
		USB2Tag_EPRes_BufferSize, RTL8821CU_RX_BUFFER_SIZE,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		TAG_END );

	if ( ! unit->unit_Bulk_In )
	{
		return ( FALSE );
	}

	for( index = 0; index < RTL8821CU_TX_PIPES; index++ )
	{
		unit->unit_Bulk_Out[index] = usb2->USB2_EPRes_ObtainTags( unit->unit_USB_Register,
			USB2Tag_EPRes_InterfaceNr, RTL8821CU_INTERFACE,
			USB2Tag_EPRes_EndPointNr, BulkOutEndpoints[index],
			USB2Tag_EPRes_BufferSize, ( index == 0 ) ? RTL8821CU_FIRMWARE_BUFFER_SIZE : RTL8821CU_TX_BUFFER_SIZE,
			USB2Tag_EPRes_AddZeroPacket, TRUE,
			TAG_END );

		if ( ! unit->unit_Bulk_Out[index] )
		{
			return ( FALSE );
		}
	}

	unit->unit_Interrupt_In = usb2->USB2_EPRes_ObtainTags( unit->unit_USB_Register,
		USB2Tag_EPRes_InterfaceNr, RTL8821CU_INTERFACE,
		USB2Tag_EPRes_EndPointNr, 0x87,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		TAG_END );

	if ( ! unit->unit_Interrupt_In )
	{
		return ( FALSE );
	}

	if ( ! RTL8821CU_Probe( unit ) )
	{
		return ( FALSE );
	}

	unit->unit_FirmwareRunning = RTL8821CU_LoadFirmware( unit );

	if ( ! unit->unit_FirmwareRunning )
	{
		return ( FALSE );
	}

	unit->unit_AddressValid = RTL8821CU_ReadStationAddress( unit );

	if ( ! unit->unit_AddressValid )
	{
		return ( FALSE );
	}

	unit->unit_RadioReady = RTL8821CU_StartRadio( unit );

	return ( unit->unit_RadioReady );
}

// --
