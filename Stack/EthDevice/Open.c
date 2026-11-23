
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
	#ifdef USB2_BIND_DEBUG
	struct Task *caller;
	#endif

	S32 error;
	U32 poll;
	#ifdef USB2_BIND_DEBUG
	S32 trace;
	#endif

	base = (PTR) Self->Data.LibBase;
	usbbase = base->USB;
	IExec = usbbase->usb_IExec;
	error = IOERR_OPENFAIL;

	if ( request->io_Message.mn_Length < sizeof( struct IORequest ) )
	{
		return ( IOERR_BADLENGTH );
	}

	request->io_Unit = NULL;

	#ifdef USB2_BIND_DEBUG
	trace = ( number != USB2_ETH_BACKEND_UNIT ) &&
		( request->io_Message.mn_Length >= sizeof( struct IOSana2Req ) );

	if ( trace )
	{
		caller = IExec->FindTask( NULL );

		IExec->DebugPrintF( "USB2 Ethernet: SANA-II open requested for unit %lu by '%s'\n",
			number, caller->tc_Node.ln_Name ? caller->tc_Node.ln_Name : "(unnamed task)" );
	}
	#endif

	if ( ( number != USB2_ETH_BACKEND_UNIT ) && ( ! USB2_FullBoot_Wait( usbbase ) ) )
	{
		#ifdef USB2_BIND_DEBUG
		if ( trace )
		{
			IExec->DebugPrintF( "USB2 Ethernet: SANA-II open for unit %lu did not pass the Full Boot wait\n", number );
		}
		#endif

		goto bailout;
	}

	IExec->ObtainSemaphore( & base->OpenLock );

	if ( ( number == USB2_ETH_BACKEND_UNIT ) ||
		( ( number == 0 ) && ( request->io_Message.mn_Length < sizeof( struct IOSana2Req ) ) ) )
	{
		goto metadata;
	}

	if ( ( number >= USB2_ETH_MAX_UNITS ) || ( request->io_Message.mn_Length < sizeof( struct IOSana2Req ) ) )
	{
		goto unlock;
	}

	// The Full Boot pass starts class drivers, which register their units asynchronously.
	for ( poll = 0; ( poll < ETH_OPEN_WAIT_POLLS ) && ( ! base->Units[number] ); poll++ )
	{
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

		MISC_WAIT( ETH_OPEN_WAIT_INTERVAL_US );

		IExec->ObtainSemaphore( & base->OpenLock );
	}

	unit = base->Units[number];

	if ( unit )
	{
		error = unit->Ops->Open( unit->Adapter, (PTR) request, flags );

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
