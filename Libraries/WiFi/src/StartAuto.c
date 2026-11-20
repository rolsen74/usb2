
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

#include <dos/dostags.h>

// --

#define WIFI_ADAPTERS 2
#define WIFI_UNITS 32

struct WiFiWorker
{
	struct DeathMessage Death;
	U32 Adapter;
	U32 Unit;
};

// Keep each slot reserved until DOS confirms that the worker has left library code.
static struct WiFiWorker Workers[WIFI_ADAPTERS][WIFI_UNITS];
static U32 Running[WIFI_ADAPTERS];

// --

S32
WiFi_WorkersActive( void )
{
	struct WiFiWorker *worker;

	while( ( worker = (APTR) GetMsg( WiFiWorkerPort ) ) )
	{
		Running[worker->Adapter] &= ~( 1UL << worker->Unit );
	}

	return ( Running[USB2WIFI_RTL8188] || Running[USB2WIFI_RT2870] );
}

// --

static S32
AutoProcess( void )
{
	struct WiFiWorker *worker;

	const char *device;

	worker = FindTask( NULL )->tc_UserData;
	device = worker->Adapter == USB2WIFI_RT2870 ? "usb2rt2870.device" : "usb2rtl8188.device";

	return ( WiFi_Run( device, worker->Unit ) );
}

// --

S32
WiFi_StartAuto( struct USB2WiFiIFace *self, U32 adapter, U32 unit )
{
	struct WiFiWorker *worker;
	struct Process *process;
	struct Library *base;

	U32 bit;

	if ( ( adapter >= WIFI_ADAPTERS ) || ( unit >= WIFI_UNITS ) )
	{
		return ( FALSE );
	}

	bit = 1UL << unit;

	ObtainSemaphore( & WiFiLock );

	WiFi_WorkersActive();

	if ( ( ActiveUnits[adapter] | Running[adapter] ) & bit )
	{
		ReleaseSemaphore( & WiFiLock );

		return ( TRUE );
	}

	Running[adapter] |= bit;
	worker = & Workers[adapter][unit];

	memset( worker, 0, sizeof( *worker ) );

	worker->Adapter = adapter;
	worker->Unit = unit;
	worker->Death.dm_Msg.mn_Node.ln_Type = NT_MESSAGE;
	worker->Death.dm_Msg.mn_ReplyPort = WiFiWorkerPort;
	worker->Death.dm_Msg.mn_Length = sizeof( worker->Death );

	ReleaseSemaphore( & WiFiLock );

	process = CreateNewProcTags( NP_Name, "USB2 Wi-Fi", NP_Entry, AutoProcess, NP_UserData, worker,
		NP_Child, FALSE, NP_StackSize, 65536, NP_NotifyOnDeathMessage, & worker->Death, TAG_END );

	if ( ! process )
	{
		ObtainSemaphore( & WiFiLock );

		Running[adapter] &= ~bit;

		ReleaseSemaphore( & WiFiLock );

		DebugPrintF( "USB2 Wi-Fi: could not start the connection task.\n" );

		return ( FALSE );
	}

	base = self->Data.LibBase;

	DebugPrintF( "USB2 Wi-Fi: usb2wifi.library %lu.%lu started the connection task for %s unit %lu.\n",
		(U32) base->lib_Version, (U32) base->lib_Revision,
		adapter == USB2WIFI_RT2870 ? "RT2870" : "RTL8188EU", unit );

	return ( TRUE );
}

// --
