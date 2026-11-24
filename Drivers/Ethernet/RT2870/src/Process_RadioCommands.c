
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Commands.h"
#include "WiFiCommand.h"
#include <usb2wifi.h>

// --

static void
ServiceReceive( struct EthernetUnit *unit )
{
	struct IOSana2Req *request;
	struct IOSana2Req *next;

	S32 error;

	error = unit->unit_Channel ? RT2870_ReadBeacon( unit ) : USB2Err_Stack_EndPointNotFound;

	if ( ! error )
	{
		return;
	}

	USBERROR( "RT2870: channel %lu wireless receive failed, USB error %ld", (U32) unit->unit_Channel, error );

	RT2870_DropLink( unit, error == USB2Err_Device_Detached );

	ObtainSemaphore( & unit->unit_CommandLock );

	Sana2_SignalEvent( & unit->unit_Pending, S2EVENT_ERROR | S2EVENT_HARDWARE );

	request = (PTR) GetHead( & unit->unit_Pending );

	while( request )
	{
		next = (PTR) GetSucc( (PTR) request );

		if ( request->ios2_Req.io_Command == S2_READMGMT )
		{
			Remove( (PTR) request );

			request->ios2_Req.io_Error = S2ERR_OUTOFSERVICE;
			request->ios2_WireError = S2WERR_RCVREL_HDW_ERR;
			request->ios2_DataLength = 0;

			ReplyMsg( (PTR) request );
		}

		request = next;
	}

	ReleaseSemaphore( & unit->unit_CommandLock );
}

// --

void
Process_RadioCommands( struct EthernetUnit *unit )
{
	struct IOSana2Req *request;

	S32 startWiFi;
	S32 receive;

	receive = unit->unit_Operational || unit->unit_ReceivePending;

	ObtainSemaphore( & unit->unit_CommandLock );

	startWiFi = unit->unit_WiFiStartPending && ! unit->unit_Detached;
	unit->unit_WiFiStartPending = FALSE;

	ReleaseSemaphore( & unit->unit_CommandLock );

	if ( startWiFi )
	{
		Wireless_StartAuto( USB2WIFI_RT2870, unit->unit_UnitNr );
	}

	ObtainSemaphore( & unit->unit_WorkLock );

	ObtainSemaphore( & unit->unit_CommandLock );

	request = (PTR) GetHead( & unit->unit_Pending );

	while( request )
	{
		if ( ( request->ios2_Req.io_Command == S2_SETOPTIONS ) ||
			( request->ios2_Req.io_Command == USB2WIRELESS_CMD_PROMPT ) ||
			( request->ios2_Req.io_Command == S2_WRITEMGMT ) ||
			( request->ios2_Req.io_Command == CMD_WRITE ) ||
			( request->ios2_Req.io_Command == S2_BROADCAST ) ||
			( request->ios2_Req.io_Command == S2_MULTICAST ) ||
			( request->ios2_Req.io_Command == S2_ONLINE ) ||
			( request->ios2_Req.io_Command == S2_OFFLINE ) ||
			( request->ios2_Req.io_Command == S2_SETKEY ) )
		{
			Remove( (PTR) request );

			break;
		}

		if ( ( request->ios2_Req.io_Command == S2_READMGMT ) || ( request->ios2_Req.io_Command == CMD_READ ) ||
			( request->ios2_Req.io_Command == S2_READORPHAN ) )
		{
			receive = TRUE;
		}

		request = (PTR) GetSucc( (PTR) request );
	}

	ReleaseSemaphore( & unit->unit_CommandLock );

	if ( request )
	{
		if ( request->ios2_Req.io_Command == S2_SETOPTIONS )
		{
			Cmd_SetOptions( unit, request );
		}
		else if ( ( request->ios2_Req.io_Command == CMD_WRITE ) ||
			( request->ios2_Req.io_Command == S2_BROADCAST ) ||
			( request->ios2_Req.io_Command == S2_MULTICAST ) )
		{
			Cmd_Write( unit, request );
		}
		else if ( request->ios2_Req.io_Command == S2_ONLINE )
		{
			Cmd_Online( unit, request );
		}
		else if ( request->ios2_Req.io_Command == S2_OFFLINE )
		{
			Cmd_Offline( unit, request );
		}
		else if ( request->ios2_Req.io_Command == USB2WIRELESS_CMD_PROMPT )
		{
			Wireless_StartPassword( request, USB2WIFI_RT2870, unit->unit_UnitNr );
		}
		else if ( request->ios2_Req.io_Command == S2_SETKEY )
		{
			Cmd_SetKey( unit, request );
		}
		else
		{
			RT2870_WriteManagement( unit, request );
		}

		ReplyMsg( (PTR) request );

		ReleaseSemaphore( & unit->unit_WorkLock );

		if ( unit->unit_Operational )
		{
			ServiceReceive( unit );
		}

		Signal( unit->unit_TaskAdr, SIGBREAKF_CTRL_E );

		return;
	}

	ReleaseSemaphore( & unit->unit_WorkLock );

	if ( ! receive )
	{
		return;
	}

	if ( RT2870_DeliverEAPOL( unit ) )
	{
		Signal( unit->unit_TaskAdr, SIGBREAKF_CTRL_E );

		return;
	}

	ServiceReceive( unit );
}

// --
