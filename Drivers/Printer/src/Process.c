
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

#include "Process_Init_1_Get_Config.c"
#include "Process_Init.c"
#include "Process_Free.c"

// --

U32
Printer_Entry( struct USB2DriverIFace *Self UNUSED, struct USB2_DriverMessage *msg )
{
	struct PrinterUnit *unit;
	struct IORequest request;

	const char *stage;
	U8 protocol;
	S32 opened;
	S32 registered;
	S32 quiesced;

	if ( ( ! msg ) || ( ! msg->IUSB2 ) || ( ! msg->Interface ) || ( ! msg->InterfaceDescriptor ) ||
		( msg->InterfaceDescriptor->InterfaceClass != USBCLASS_Printer ) ||
		( msg->InterfaceDescriptor->InterfaceSubClass != USBPRT_SUBCLASS_Printer ) )
	{
		return ( FALSE );
	}

	protocol = msg->InterfaceDescriptor->InterfaceProtocol;

	if ( ( protocol < USBPRT_PROTOCOL_UNIDIRECTIONAL ) || ( protocol > USBPRT_PROTOCOL_IEEE1284_4 ) )
	{
		return ( FALSE );
	}

	IUSB2 = msg->IUSB2;
	opened = FALSE;
	registered = FALSE;
	quiesced = FALSE;
	stage = "allocate unit";

	unit = AllocVecTags( sizeof( *unit ), AVT_Clear, 0, TAG_END );

	if ( ! unit )
	{
		goto bailout;
	}

	unit->unit_USB_Interface = msg->Interface;
	unit->unit_Dot4_Active = protocol == USBPRT_PROTOCOL_IEEE1284_4;
	stage = "open " USB2_PRINTER_DEVICE;

	memset( & request, 0, sizeof( request ) );

	request.io_Message.mn_Length = sizeof( request );

	if ( OpenDevice( USB2_PRINTER_DEVICE, USB2_PORT_BACKEND_UNIT, & request, 0 ) )
	{
		goto bailout;
	}

	opened = TRUE;
	stage = "obtain printer registration interface";

	unit->unit_PortInterface = (PTR) GetInterface( (PTR) request.io_Device, "main", 1, NULL );

	if ( ! unit->unit_PortInterface )
	{
		goto bailout;
	}

	stage = "initialize printer endpoints";

	if ( ! __myProcess_Init( unit, msg ) )
	{
		goto bailout;
	}

	stage = "register printer unit";

	if ( request.io_Device->dd_Library.lib_Revision >= USB2_PRINTER_RECONNECT_REVISION )
	{
		unit->unit_PortUnit = unit->unit_PortInterface->RegisterDevice(
			unit->unit_PRT_Begin_MsgPort, unit->unit_PRT_Abort_MsgPort, &unit->unit_UnitNr, msg );
	}
	else
	{
		unit->unit_PortUnit = unit->unit_PortInterface->Register(
			unit->unit_PRT_Begin_MsgPort, unit->unit_PRT_Abort_MsgPort, &unit->unit_UnitNr );
	}

	if ( ! unit->unit_PortUnit )
	{
		goto bailout;
	}

	registered = TRUE;

	__myProcess_1_Get_Config( unit );

	DebugPrintF( "USB2 printer: %04lX:%04lX registered " USB2_PRINTER_DEVICE " unit %lu\n",
		(ULONG) msg->VendorID, (ULONG) msg->DeviceID, unit->unit_UnitNr );

	Process_Main( unit );

bailout:

	if ( ! registered )
	{
		DebugPrintF( "USB2 printer: %04lX:%04lX could not %s\n",
			(ULONG) msg->VendorID, (ULONG) msg->DeviceID, stage );
	}

	if ( unit )
	{
		if ( ( unit->unit_PortUnit ) && ( request.io_Device->dd_Library.lib_Revision >= 8 ) )
		{
			unit->unit_PortInterface->Quiesce( unit->unit_PortUnit );
			quiesced = TRUE;
		}

		__myProcess_CleanupDevice( unit );

		if ( quiesced )
		{
			// Reply to admitted work while the bridge still owns both ports.
			__myProcess_Free_Lists( unit );

			__myProcess_Drain_MsgPort( unit );
		}

		if ( unit->unit_PortUnit )
		{
			unit->unit_PortInterface->Unregister( unit->unit_PortUnit );
		}

		__myProcess_Free( unit );

		if ( unit->unit_PortInterface )
		{
			DropInterface( (PTR) unit->unit_PortInterface );
		}

		FreeVec( unit );
	}

	if ( opened )
	{
		CloseDevice( & request );
	}

	return ( registered );
}

// --
