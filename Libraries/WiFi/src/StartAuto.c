
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

struct WiFiWorker
{
	struct DeathMessage Death;
	struct WiFiWorker *Next;
	const char *Device;
	U32 Adapter;
	U32 Unit;
};

static struct WiFiWorker *Workers;

// --

static void
RemoveWorker( struct WiFiWorker *worker )
{
	struct WiFiWorker **link;

	link = &Workers;

	while ( *link != worker )
	{
		link = &(*link)->Next;
	}

	*link = worker->Next;

	FreeVec( worker );
}

// --

static void
ReapWorkers( void )
{
	struct WiFiWorker *worker;

	while ( ( worker = (APTR)GetMsg( WiFiWorkerPort ) ) )
	{
		RemoveWorker( worker );
	}
}

// --

S32
WiFi_WorkersActive( void )
{
	S32 active;

	ObtainSemaphore( &WiFiLock );

	ReapWorkers();

	active = Workers != NULL;

	ReleaseSemaphore( &WiFiLock );

	return ( active );
}

// --

static S32
AutoProcess( void )
{
	struct WiFiWorker *worker;

	worker = FindTask( NULL )->tc_UserData;

	return ( WiFi_Run( worker->Device, worker->Unit, FALSE ) );
}

// --

S32
WiFi_StartAuto( struct USB2WiFiIFace *self, U32 adapter, U32 unit )
{
	struct WiFiWorker *worker;
	struct Process *process;
	struct Library *base;

	const char *device;

	device = WiFi_DeviceName( adapter );

	if ( ! device )
	{
		return ( FALSE );
	}

	ObtainSemaphore( &WiFiLock );

	ReapWorkers();

	for ( worker = Workers; worker; worker = worker->Next )
	{
		if ( ( worker->Adapter == adapter ) && ( worker->Unit == unit ) )
		{
			break;
		}
	}

	if ( ( worker ) || ( WiFi_UnitClaimed( adapter, unit ) ) )
	{
		ReleaseSemaphore( &WiFiLock );

		return ( TRUE );
	}

	worker = AllocVecTags( sizeof( *worker ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! worker )
	{
		ReleaseSemaphore( &WiFiLock );

		return ( FALSE );
	}

	worker->Next = Workers;
	Workers = worker;
	worker->Device = device;
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

		RemoveWorker( worker );

		ReleaseSemaphore( & WiFiLock );

		DebugPrintF( "USB2 Wi-Fi: could not start the connection task.\n" );

		return ( FALSE );
	}

	base = self->Data.LibBase;

	DebugPrintF( "USB2 Wi-Fi: usb2wifi.library %lu.%lu started the connection task for %s unit %lu.\n",
		(U32) base->lib_Version, (U32) base->lib_Revision,
		device, unit );

	return ( TRUE );
}

// --
