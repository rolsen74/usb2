
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Controller.h"

// --

static int
ControlRequest( struct devUnit *unit, U8 type, U8 request, U16 value, U16 index, U8 *data, U16 length )
{
	struct USB2_SetupData *setup;
	struct USB2_IORequest *ioreq;

	ioreq = unit->unit_USBControlIOReq;
	setup = unit->unit_USBRegister->Res_Control->SetupData;
	setup->RequestType = type;
	setup->RequestCode = request;
	setup->Value = LE_SWAP16( value );
	setup->Index = LE_SWAP16( index );
	setup->Length = LE_SWAP16( length );
	ioreq->io_Command = ( type & REQTYPE_Read ) ? CMD_READ : CMD_WRITE;
	ioreq->io_Length = length;
	ioreq->io_Data = data;
	ioreq->io_SetupData = setup;
	ioreq->io_SetupLength = sizeof( *setup );
	ioreq->io_TimeOut = 1000000;

	IExec->DoIO( (PTR) ioreq );

	ioreq->io_Data = NULL;

	return ( ioreq->io_Error ? ioreq->io_Error :
		( ioreq->io_Actual == length ? 0 : IOERR_BADLENGTH ) );
}

// --

int
RTL8152_Control( struct devUnit *unit, U32 command, U16 address, U16 index, U8 *data, U16 length )
{
	U8 type;

	if ( unit->unit_USBControlError )
	{
		return ( unit->unit_USBControlError );
	}

	type = REQTYPE_Vendor | REQTYPE_Device | ( command == CMD_READ ? REQTYPE_Read : REQTYPE_Write );

	unit->unit_USBControlError = ControlRequest( unit, type, 0x05, address, index, data, length );

	if ( unit->unit_USBControlError )
	{
		IExec->DebugPrintF( "RTL8152: register $%04lx index $%04lx %s failed, error %ld, %lu/%lu bytes\n",
			(U32) address, (U32) index, command == CMD_READ ? "read" : "write",
			unit->unit_USBControlError, unit->unit_USBControlIOReq->io_Actual, (U32) length );
	}

	return ( unit->unit_USBControlError );
}

// --

void
RTL8152_ReportUSBConfiguration( struct devUnit *unit )
{
	struct USB2IFace *IUSB2;

	U32 controller;
	U32 speed;
	U8 config;
	S32 error;

	IUSB2 = unit->unit_IUSB2;
	controller = HCDTYPE_Last;
	speed = -1U;

	IUSB2->USB2_Attr_Get( unit->unit_USBRegister->FKT_ID, USB2Tag_Attr_HCDType, & controller, sizeof( controller ) );

	IUSB2->USB2_Attr_Get( unit->unit_USBRegister->FKT_ID, USB2Tag_Attr_Speed, & speed, sizeof( speed ) );

	IExec->DebugPrintF( "RTL8152: USB controller type %lu, speed %lu, EP0 max packet %lu, interface %lu\n",
		controller, speed, unit->unit_USBRegister->Res_Control->EndPointMax, unit->unit_USBInterface->IFC_Num );

	error = ControlRequest( unit, REQTYPE_Read | REQTYPE_Standard | REQTYPE_Device,
		REQCODE_Get_Configuration, 0, 0, & config, sizeof( config ) );

	if ( error )
	{
		IExec->DebugPrintF( "RTL8152: USB GET_CONFIGURATION failed, error %ld\n", error );
	}
	else
	{
		IExec->DebugPrintF( "RTL8152: USB GET_CONFIGURATION returned %lu, interface %lu\n",
			(U32) config, unit->unit_USBInterface->IFC_Num );
	}
}

// --
