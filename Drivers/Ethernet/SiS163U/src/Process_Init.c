
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static struct USB2_Interface *
FindInterface( void )
{
	struct USB2_Interface *ifc;

	U32 index;

	for( index = 0;; index++ )
	{
		ifc = USB2_Ifc_FindTags( USB2Tag_Find_VendorID, SIS163U_VENDOR,
			USB2Tag_Find_DeviceID, SIS163U_PRODUCT, USB2Tag_Find_Class, USBCLASS_Vendor_Specific,
			USB2Tag_Find_Index, index, TAG_END );

		if ( ( ! ifc ) || ( ifc->IFC_Num == 0 ) )
		{
			return ( ifc );
		}
	}
}

// --

S32
Process_Init( struct EthernetUnit *unit )
{
	struct USB2_Device_Desc desc;
	struct USB2_Interface *ifc;
	struct USB2_Register *reg;

	U32 value;
	S32 error;

	ifc = FindInterface();

	if ( ! ifc )
	{
		USBERROR( "SiS163U: no available 0D8E:0163 interface 0" );

		return ( FALSE );
	}

	reg = USB2_Reg_RegisterTags( USB2Tag_Reg_Interface, ifc,
		USB2Tag_Reg_Title, "SiS163U", TAG_END );

	if ( ! reg )
	{
		USBERROR( "SiS163U: USB interface registration failed" );

		return ( FALSE );
	}

	unit->unit_USB_Register = reg;

	if ( USB2_Attr_Get( ifc->FKT_ID, USB2Tag_Attr_DeviceDesc, & desc, sizeof( desc ) ) == sizeof( desc ) )
	{
		unit->unit_ChipRevision = ( LE_SWAP16( desc.DeviceVers ) >> 8 ) + 0x30;

		USBINFO( "SiS163U: bcdDevice $%04lx, chip revision $%02lx",
			(U32) LE_SWAP16( desc.DeviceVers ), (U32) unit->unit_ChipRevision );
	}

	error = SiS163U_ReadRegister( reg->Res_Control, 0x00c8, & value );

	if ( error )
	{
		USBERROR( "SiS163U: unit %lu register $00C8 read failed, USB error %ld", unit->unit_UnitNr, error );

		return ( FALSE );
	}

	unit->unit_Bulk_Rx_Resource = USB2_EPRes_ObtainTags( reg, USB2Tag_EPRes_InterfaceNr, 0,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In, USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_NrOfIORequest, 1, USB2Tag_EPRes_BufferSize, SIS163U_RX_BUFFER_SIZE,
		USB2Tag_EPRes_AllowShortPackets, TRUE, USB2Tag_EPRes_TimeOut, 400000, TAG_END );

	unit->unit_Bulk_Tx_Resource = USB2_EPRes_ObtainTags( reg, USB2Tag_EPRes_InterfaceNr, 0,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_Out, USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_NrOfIORequest, 0, TAG_END );

	if ( ( ! unit->unit_Bulk_Rx_Resource ) || ( ! unit->unit_Bulk_Tx_Resource ) )
	{
		USBERROR( "SiS163U: unit %lu could not obtain both bulk endpoints", unit->unit_UnitNr );

		return ( FALSE );
	}

	USBINFO( "SiS163U: unit %lu 0D8E:0163 register $00C8 = $%08lx, bulk IN %lu and OUT %lu bytes/packet",
		unit->unit_UnitNr, value, unit->unit_Bulk_Rx_Resource->EndPointMax, unit->unit_Bulk_Tx_Resource->EndPointMax );

	unit->unit_StationAddressValid = SiS163U_ReadStationAddress( unit );

	if ( unit->unit_StationAddressValid )
	{
		if ( unit->unit_RFType == 0x08 )
		{
			unit->unit_RadioInitialized = SiS163U_StartRadio( unit );
		}

		unit->unit_StationAddressProgrammed = SiS163U_ProgramStationAddress( unit );
	}

	return ( TRUE );
}

// --
