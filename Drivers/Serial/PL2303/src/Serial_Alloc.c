/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE struct SerialUnit *
Serial_Alloc ( struct SerialContext * ctx, struct USB2_DriverMessage * msg )
{
	struct SerialUnit *		unit;
	struct USB2_Interface * data;

	const char * stage;
	U32			 control_nr;
	U32			 data_nr;
	U32			 alt;
	ULONG		 number;
	S32			 error;

	unit = AllocVecTags ( sizeof ( struct SerialUnit ), AVT_Clear, 0, TAG_END );

	if ( ! unit )
	{
		DebugPrintF ( "USB2 serial: %04lX:%04lX could not allocate unit\n", (U32)msg->VendorID, (U32)msg->DeviceID );

		return ( NULL );
	}

	unit->StartMsg = msg;
	unit->ACM	   = ( msg->VendorID != SERIAL_PL2303_VENDOR ) || ( msg->DeviceID != SERIAL_PL2303_PRODUCT );
	control_nr	   = msg->InterfaceDescriptor->InterfaceNumber;
	data_nr		   = control_nr;
	alt			   = 0;

	DebugPrintF ( "USB2 serial: probing %04lX:%04lX interface %lu (%s)\n", (U32)msg->VendorID, (U32)msg->DeviceID, control_nr,
				  unit->ACM ? "CDC ACM" : "PL2303" );

	NewList ( &unit->ReadList );
	NewList ( &unit->WriteList );

	stage = "parse CDC ACM descriptors";

	if ( ( unit->ACM ) && ( ! Serial_ACM_Parse ( unit, &data_nr, &alt ) ) )
	{
		goto bailout;
	}

	stage = "allocate serial message ports";

	if ( ! ( unit->BeginPort = AllocSysObjectTags ( ASOT_PORT, TAG_END ) ) )
	{
		goto bailout;
	}

	if ( ! ( unit->AbortPort = AllocSysObjectTags ( ASOT_PORT, TAG_END ) ) )
	{
		goto bailout;
	}

	stage = "register USB interface";

	unit->Register = USB2_Reg_RegisterTags ( USB2Tag_Reg_DriverMessage, msg, USB2Tag_Reg_Title,
											 unit->ACM ? "CDC ACM Serial" : "PL2303 Serial", TAG_END );

	if ( ! unit->Register )
	{
		goto bailout;
	}

	unit->Control = unit->Register->Res_Control;
	stage		  = "obtain control endpoint";

	if ( ( ! unit->Control ) || ( ! unit->Control->SetupData ) || ( ! unit->Control->IORequests )
		 || ( ! unit->Control->IORequests[0] ) )
	{
		goto bailout;
	}

	if ( unit->ACM )
	{
		stage = "find CDC data interface";

		data = USB2_Ifc_FindTags ( USB2Tag_Find_FunctionID, &unit->Register->FKT_ID, USB2Tag_Find_InterfaceNumber, data_nr,
								   TAG_END );

		if ( ! data )
		{
			goto bailout;
		}

		stage = "claim CDC data interface";

		if ( ! USB2_Ifc_Claim ( unit->Register, data ) )
		{
			goto bailout;
		}

		unit->DataInterface = data;
		stage				= "select CDC data alternate setting";

		if ( ( alt ) && ( ! USB2_Ifc_SetAlt ( unit->Register, data, alt ) ) )
		{
			goto bailout;
		}

		unit->DataAlt = alt;
	}

	stage = "obtain bulk IN endpoint";

	unit->BulkIn
		= USB2_EPRes_ObtainTags ( unit->Register, USB2Tag_EPRes_InterfaceNr, data_nr, USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
								  USB2Tag_EPRes_EPDirection, EPADR_Dir_In, USB2Tag_EPRes_AllowShortPackets, TRUE,
								  USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize, TAG_END );

	if ( ! unit->BulkIn )
	{
		goto bailout;
	}

	stage = "obtain bulk OUT endpoint";

	unit->BulkOut = USB2_EPRes_ObtainTags ( unit->Register, USB2Tag_EPRes_InterfaceNr, data_nr, USB2Tag_EPRes_EPType,
											EPATT_Type_Bulk, USB2Tag_EPRes_EPDirection, EPADR_Dir_Out, TAG_END );

	if ( ! unit->BulkOut )
	{
		goto bailout;
	}

	stage = "obtain interrupt IN endpoint";

	unit->Interrupt = USB2_EPRes_ObtainTags ( unit->Register, USB2Tag_EPRes_InterfaceNr, control_nr, USB2Tag_EPRes_EPType,
											  EPATT_Type_Interrupt, USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
											  USB2Tag_EPRes_BufferSize, USB2Val_BufferSize_MaxPacketSize, TAG_END );

	if ( ( ! unit->Interrupt ) && ( ! unit->ACM ) )
	{
		goto bailout;
	}

	stage = "initialize adapter";

	if ( ! Serial_Startup ( ctx, unit ) )
	{
		goto bailout;
	}

	unit->PortRequest.io_Message.mn_Length = sizeof ( unit->PortRequest );
	stage								   = "open " USB2_SERIAL_DEVICE " backend";

	error = OpenDevice ( USB2_SERIAL_DEVICE, USB2_PORT_BACKEND_UNIT, &unit->PortRequest, 0 );

	if ( error )
	{
		DebugPrintF ( "USB2 serial: OpenDevice returned %ld\n", error );

		unit->PortRequest.io_Device = NULL;
		goto bailout;
	}

	stage = "get " USB2_SERIAL_DEVICE " interface";

	unit->PortInterface = (PTR)GetInterface( (PTR)unit->PortRequest.io_Device, "main", 1, NULL );

	if ( ! unit->PortInterface )
	{
		goto bailout;
	}

	unit->SerialInterface = (PTR)GetInterface( (PTR)unit->PortRequest.io_Device, "serial", 1, NULL );

	Serial_BreakInit( unit );

	stage = "register serial unit";

	unit->PortUnit = unit->PortInterface->Register ( unit->BeginPort, unit->AbortPort, &number );

	if ( ! unit->PortUnit )
	{
		goto bailout;
	}

	DebugPrintF ( "USB2 serial: %04lX:%04lX registered " USB2_SERIAL_DEVICE " unit %lu\n", (U32)msg->VendorID,
				  (U32)msg->DeviceID, number );

	return ( unit );

bailout:
	DebugPrintF ( "USB2 serial: %04lX:%04lX interface %lu could not %s\n", (U32)msg->VendorID, (U32)msg->DeviceID, control_nr,
				  stage );

	Serial_Free ( ctx, unit );

	return ( NULL );
}

// --
