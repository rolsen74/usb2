
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "EthDevice.h"

// --

#define ETH_OPEN_WAIT_INTERVAL_US 100000
#define ETH_OPEN_WAIT_POLLS 30

// --

SEC_CODE S32
EthDevice_Open( struct DeviceManagerInterface *Self, struct IORequest *request, U32 number, U32 flags )
{
	struct USB2EthDevice *base;
	struct USB2EthUnit *unit;
	struct ExecIFace *IExec;
	struct USBBase *usbbase;
	struct USB2Prefs prefs;
	#ifdef USB2_BIND_DEBUG
	struct Task *caller;
	#endif

	S32 error;
	S32 follow_default;
	S32 select_adapter;
	S32 first;
	S32 destroy;
	U32 poll;
	U32 index;
	#ifdef USB2_BIND_DEBUG
	S32 trace;
	#endif

	base = (PTR) Self->Data.LibBase;
	usbbase = base->USB;
	IExec = usbbase->usb_IExec;
	error = IOERR_OPENFAIL;
	follow_default = ( number == USB2_DEFAULT_UNIT );
	select_adapter = FALSE;
	first = FALSE;

	if ( request->io_Message.mn_Length < sizeof( struct IORequest ) )
	{
		return ( IOERR_BADLENGTH );
	}

	request->io_Unit = NULL;

	#ifdef USB2_BIND_DEBUG
	trace = ( number != USB2_ETH_BACKEND_UNIT ) &&
		( request->io_Message.mn_Length >= sizeof( struct IOSana2Req ) );
	#endif

	if ( ( number == USB2_ETH_BACKEND_UNIT ) ||
		( ( number == 0 ) && ( request->io_Message.mn_Length < sizeof( struct IOSana2Req ) ) ) )
	{
		IExec->ObtainSemaphore( & base->OpenLock );

		goto metadata;
	}

	#ifdef USB2_BIND_DEBUG
	if ( trace )
	{
		caller = IExec->FindTask( NULL );

		IExec->DebugPrintF( "USB2 Ethernet: SANA-II open requested for unit %lu by '%s'\n",
			number, caller->tc_Node.ln_Name ? caller->tc_Node.ln_Name : "(unnamed task)" );
	}
	#endif

	if ( ( follow_default ) && ( IExec->FindTask( NULL )->tc_Node.ln_Type != NT_PROCESS ) )
	{
		goto bailout;
	}

	if ( ! USB2_FullBoot_Wait( usbbase ) )
	{
		if ( ( number == 0 ) && ( usbbase->usb_FullBootState == USB2_FullBoot_Failed ) )
		{
			IExec->ObtainSemaphore( & base->OpenLock );

			goto metadata;
		}

		#ifdef USB2_BIND_DEBUG
		if ( trace )
		{
			IExec->DebugPrintF( "USB2 Ethernet: SANA-II open for unit %lu did not pass the Full Boot wait\n", number );
		}
		#endif

		goto bailout;
	}

	if ( follow_default )
	{
		USB2_ReadPrefs( usbbase->usb_IDOS, & prefs );

		select_adapter = ( prefs.Ethernet[0] ) && ( MISC_STRICASECOMPARE( prefs.Ethernet, USB2_PREF_AUTO ) );

		if ( select_adapter )
		{
			first = ! MISC_STRICASECOMPARE( prefs.Ethernet, USB2_PREF_FIRST );
		}

		number = 0;
	}

	IExec->ObtainSemaphore( & base->OpenLock );

	if ( ( number >= USB2_ETH_MAX_UNITS ) || ( request->io_Message.mn_Length < sizeof( struct IOSana2Req ) ) )
	{
		goto unlock;
	}

	// Class drivers register asynchronously after Full Boot.
	for( poll = 0; ; poll++ )
	{
		unit = base->Units[number];

		if ( select_adapter )
		{
			for( index = 0; index < USB2_ETH_MAX_UNITS; index++ )
			{
				unit = base->Units[index];

				if ( ( unit ) && ( ( first ) || ( ! MISC_STRICASECOMPARE( unit->Info.Driver, prefs.Ethernet ) ) ) )
				{
					break;
				}

				unit = NULL;
			}
		}

		if ( ( unit ) || ( poll == ETH_OPEN_WAIT_POLLS ) )
		{
			break;
		}

		IExec->ReleaseSemaphore( & base->OpenLock );

		#ifdef USB2_BIND_DEBUG
		if ( ! poll )
		{
			IExec->DebugPrintF( "USB2 Ethernet: waiting for SANA-II unit %lu to register\n", number );
		}
		#endif

		if ( IExec->SetSignal( 0, SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C )
		{
			goto bailout;
		}

		if ( MISC_WAIT( ETH_OPEN_WAIT_INTERVAL_US ) )
		{
			goto bailout;
		}

		IExec->ObtainSemaphore( & base->OpenLock );
	}

	if ( ( ! unit ) && ( number == 0 ) && ( ! follow_default ) )
	{
		goto metadata;
	}

	if ( unit )
	{
		// Keep the selected unit alive while the adapter acquires its own lock.
		unit->Opening++;

		IExec->ReleaseSemaphore( & base->OpenLock );

		error = unit->Ops->Open( unit->Adapter, (PTR) request, flags );

		IExec->ObtainSemaphore( & base->OpenLock );

		unit->Opening--;

		if ( ( error ) && ( unit->Detached ) )
		{
			base->Reserved[unit->Number]--;
		}

		if ( ! error )
		{
			IExec->ObtainSemaphore( & base->RouteLock );

			unit->Opens++;
			base->Library.lib_OpenCnt++;
			request->io_Device = (PTR) base;
			request->io_Unit = (PTR) unit;
			request->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

			IExec->ReleaseSemaphore( & base->RouteLock );
		}

		destroy = ( unit->Detached ) && ( ! unit->Opens ) && ( ! unit->Opening );

		if ( destroy )
		{
			IExec->ReleaseSemaphore( & base->OpenLock );

			EthDevice_DestroyUnit( IExec, unit );

			goto bailout;
		}
	}

	goto unlock;

metadata:
	IExec->ObtainSemaphore( & base->RouteLock );

	base->Library.lib_OpenCnt++;
	request->io_Device = (PTR) base;
	request->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
	error = 0;

	IExec->ReleaseSemaphore( & base->RouteLock );

unlock:
	IExec->ReleaseSemaphore( & base->OpenLock );

bailout:
	if ( error )
	{
		request->io_Device = NULL;
		request->io_Unit = NULL;
	}

	request->io_Error = error;

	#ifdef USB2_BIND_DEBUG
	if ( trace )
	{
		IExec->DebugPrintF( "USB2 Ethernet: SANA-II open for unit %lu returned %ld (%s)\n",
			number, error, request->io_Unit ? "adapter attached" : "no adapter open" );
	}
	#endif

	return ( error );
}

// --
