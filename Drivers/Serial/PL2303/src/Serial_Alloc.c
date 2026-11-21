/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE struct SerialUnit *Serial_Alloc( struct SerialContext *ctx, struct USB2_DriverMessage *msg )
{
struct SerialUnit *unit;
struct USB2_Interface *data;

U32 control_nr;
U32 data_nr;
U32 alt;
U32 index;
ULONG number;

	unit = AllocVecTags( sizeof( struct SerialUnit ), AVT_Clear, 0, TAG_END );

	if ( ! unit )
	{
		return ( NULL );
	}

	unit->StartMsg = msg;
	unit->ACM = ( msg->VendorID != SERIAL_PL2303_VENDOR ) || ( msg->DeviceID != SERIAL_PL2303_PRODUCT );
	control_nr = msg->InterfaceDescriptor->InterfaceNumber;
	data_nr = control_nr;
	alt = 0;

	NewList( & unit->ReadList );
	NewList( & unit->WriteList );

	if ( ( unit->ACM ) && ( ! Serial_ACM_Parse( unit, & data_nr, & alt ) ) )
	{
		goto bailout;
	}

	if ( ! ( unit->BeginPort = AllocSysObjectTags( ASOT_PORT, TAG_END ) ) )
	{
		goto bailout;
	}

	if ( ! ( unit->AbortPort = AllocSysObjectTags( ASOT_PORT, TAG_END ) ) )
	{
		goto bailout;
	}

	unit->Register = USB2_Reg_RegisterTags(
		USB2Tag_Reg_DriverMessage, msg,
		USB2Tag_Reg_Title, unit->ACM ? "CDC ACM Serial" : "PL2303 Serial",
		TAG_END
	);

	if ( ! unit->Register )
	{
		goto bailout;
	}

	unit->Control = unit->Register->Res_Control;

	if ( ( ! unit->Control ) || ( ! unit->Control->SetupData ) ||
		( ! unit->Control->IORequests ) || ( ! unit->Control->IORequests[0] ) )
	{
		goto bailout;
	}

	if ( unit->ACM )
	{
		for( index = 0; ; index++ )
		{
			data = USB2_Ifc_FindTags(
				USB2Tag_Find_VendorID, msg->VendorID,
				USB2Tag_Find_DeviceID, msg->DeviceID,
				USB2Tag_Find_Class, USBCLASS_CDC_Data,
				USB2Tag_Find_Index, index,
				TAG_END );

			if ( ! data )
			{
				goto bailout;
			}

			if ( ( data->FKT_ID == msg->Interface->FKT_ID ) && ( data->IFC_Num == data_nr ) )
			{
				break;
			}
		}

		if ( ! USB2_Ifc_Claim( unit->Register, data ) )
		{
			goto bailout;
		}

		unit->DataInterface = data;

		if ( ( alt ) && ( ! USB2_Ifc_SetAlt( unit->Register, data, alt ) ) )
		{
			goto bailout;
		}

		unit->DataAlt = alt;
	}

	unit->BulkIn = USB2_EPRes_ObtainTags( unit->Register,
		USB2Tag_EPRes_InterfaceNr, data_nr,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize,
		TAG_END
	);

	if ( ! unit->BulkIn )
	{
		goto bailout;
	}

	unit->BulkOut = USB2_EPRes_ObtainTags( unit->Register,
		USB2Tag_EPRes_InterfaceNr, data_nr,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_Out,
		TAG_END
	);

	if ( ! unit->BulkOut )
	{
		goto bailout;
	}

	unit->Interrupt = USB2_EPRes_ObtainTags( unit->Register,
		USB2Tag_EPRes_InterfaceNr, control_nr,
		USB2Tag_EPRes_EPType, EPATT_Type_Interrupt,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize,
		TAG_END
	);

	if ( ( ! unit->Interrupt ) && ( ! unit->ACM ) )
	{
		goto bailout;
	}

	if ( ! Serial_Startup( ctx, unit ) )
	{
		goto bailout;
	}

	unit->PortRequest.io_Message.mn_Length = sizeof( unit->PortRequest );

	if ( OpenDevice( USB2_SERIAL_DEVICE, USB2_PORT_BACKEND_UNIT, & unit->PortRequest, 0 ) )
	{
		unit->PortRequest.io_Device = NULL;
		goto bailout;
	}

	unit->PortInterface = (PTR) GetInterface( (PTR) unit->PortRequest.io_Device, "main", 1, NULL );

	if ( ! unit->PortInterface )
	{
		goto bailout;
	}

	unit->PortUnit = unit->PortInterface->Register( unit->BeginPort, unit->AbortPort, & number );

	if ( ! unit->PortUnit )
	{
		goto bailout;
	}

	return ( unit );

bailout:
	Serial_Free( ctx, unit );

	return ( NULL );
}

// --
