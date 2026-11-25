
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "PortDevice.h"

// --

SEC_CODE static U8
SharedOpen( struct USB2PortDevice *base, struct IORequest *request )
{
	if ( base->Type == PORT_SERIAL )
	{
		return ( ( ((struct IOExtSer *) request)->io_SerFlags & SERF_SHARED ) != 0 );
	}

	return ( ( base->Type == PORT_PARALLEL ) && ( ((struct IOExtPar *) request)->io_ParFlags & PARF_SHARED ) );
}

// --

SEC_CODE static void
SerialDefaults( struct IOExtSer *request )
{
	request->io_CtlChar = SER_DEFAULT_CTLCHAR;
	request->io_RBufLen = 4096;
	request->io_ExtFlags = 0;
	request->io_SerFlags &= ~( SERF_PARTY_ON | SERF_PARTY_ODD );
	request->io_Baud = 9600;
	request->io_BrkTime = 250000;
	request->io_ReadLen = 8;
	request->io_WriteLen = 8;
	request->io_StopBits = 1;
	request->io_TermArray.TermArray0 = 0;
	request->io_TermArray.TermArray1 = 0;
}

// --

SEC_CODE static void
CopyLineParams( struct IOExtSer *dst, const struct IOExtSer *src )
{
	dst->io_Baud = src->io_Baud;
	dst->io_ReadLen = src->io_ReadLen;
	dst->io_WriteLen = src->io_WriteLen;
	dst->io_StopBits = src->io_StopBits;
	dst->io_SerFlags = ( dst->io_SerFlags & ~( SERF_PARTY_ON | SERF_PARTY_ODD ) ) |
		( src->io_SerFlags & ( SERF_PARTY_ON | SERF_PARTY_ODD ) );
	dst->io_ExtFlags = src->io_ExtFlags & ( SEXTF_MSPON | SEXTF_MARK );
}

// --

SEC_CODE void
PortDevice_SetParams( struct USB2SerialIFace *Self, struct USB2PortUnit *unit, const struct IOExtSer *params )
{
	struct USB2PortDevice *base;
	struct ExecIFace *IExec;

	base = (PTR)Self->Data.LibBase;
	IExec = base->IExec;

	IExec->ObtainSemaphore( &base->Lock );

	CopyLineParams( &unit->SerialParams, params );

	unit->SerialParamsValid = TRUE;

	IExec->ReleaseSemaphore( &base->Lock );
}

// --

SEC_CODE S32
PortDevice_Open( struct DeviceManagerInterface *Self, struct IORequest *request, U32 number, U32 flags UNUSED )
{
	struct USB2PortDevice *base;
	struct USBBase *usbbase;
	struct USB2PortUnit *unit;
	struct ExecIFace *IExec;
	struct USB2Prefs prefs;
	enum USB2DefaultDevice preference;

	S32 wait_error;
	U8 shared;
	U32 poll;
	U32 size;
	S32 metadata;
	S32 default_open;
	S32 first;

	base = (PTR) Self->Data.LibBase;
	IExec = base->IExec;
	usbbase = base->USB;
	unit = NULL;
	request->io_Unit = NULL;
	request->io_Error = IOERR_OPENFAIL;

	switch( base->Type )
	{
		case PORT_SERIAL: size = sizeof( struct IOExtSer ); break;
		case PORT_PARALLEL: size = sizeof( struct IOExtPar ); break;
		default: size = sizeof( struct IOStdReq ); break;
	}

	default_open = ( number == USB2_DEFAULT_UNIT );
	metadata = ( number == USB2_PORT_BACKEND_UNIT ) ||
		( ( number == 0 ) && ( request->io_Message.mn_Length >= sizeof( struct IOStdReq ) ) &&
			( request->io_Message.mn_Length < size ) );

	if ( ( ! metadata ) && ( request->io_Message.mn_Length < size ) )
	{
		request->io_Device = NULL;

		return ( request->io_Error );
	}

	if ( default_open )
	{
		switch( base->Type )
		{
			case PORT_SERIAL: preference = USB2_DEFAULT_SERIAL; break;
			case PORT_PARALLEL: preference = USB2_DEFAULT_PARALLEL; break;
			default: preference = USB2_DEFAULT_PRINTER; break;
		}

		if ( ( IExec->FindTask( NULL )->tc_Node.ln_Type != NT_PROCESS ) || ( ! USB2_FullBoot_Wait( usbbase ) ) )
		{
			request->io_Device = NULL;

			return ( request->io_Error );
		}

		USB2_ReadPrefs( usbbase->usb_IDOS, & prefs );

		number = ( prefs.Units[preference] == USB2_UNIT_FIRST ) ? USB2_FIRST_UNIT : (U32) MAX( 0, prefs.Units[preference] );
	}

	// Unit 0 also serves discovery callers before the first backend registers.
	if ( ( number != 0 ) && ( ! metadata ) && ( ! default_open ) && ( ! USB2_FullBoot_Wait( usbbase ) ) )
	{
		request->io_Device = NULL;

		return ( request->io_Error );
	}

	first = ( default_open ) && ( number == USB2_FIRST_UNIT );

	IExec->ObtainSemaphore( & base->Lock );

	if ( ! metadata )
	{
		for ( poll = 0; ; poll++ )
		{
			unit = base->Units;

			while( ( unit ) && ( first ? ( ! unit->BeginPort || unit->Quiescing ) : ( unit->Number < number ) ) )
			{
				unit = unit->Next;
			}

			if ( ( ( unit ) && ( ( first ) || ( unit->Number == number ) ) ) || ( number == 0 ) || ( poll == 30 ) )
			{
				break;
			}

			IExec->ReleaseSemaphore( & base->Lock );

			if ( IExec->SetSignal( 0, SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C )
			{
				request->io_Device = NULL;

				return ( request->io_Error );
			}

			wait_error = MISC_WAIT( 100000 );

			IExec->ObtainSemaphore( &base->Lock );

			if ( wait_error )
			{
				unit = NULL;

				break;
			}
		}

		if ( ( ! unit ) || ( ( ! first ) && ( unit->Number != number ) ) )
		{
			unit = NULL;

			if ( ( number == 0 ) && ( ! default_open ) )
			{
				goto opened;
			}

			goto bailout;
		}

		if ( ( unit->SerialSessions ) && ( ((struct IOExtSer *)request)->io_SerFlags & SERF_7WIRE ) )
		{
			goto bailout;
		}

		shared = SharedOpen( base, request );

		if ( ( unit->Closing ) || ( unit->Quiescing ) || ( ( unit->Opens ) && ( ( ! unit->Shared ) || ( ! shared ) ) ) )
		{
			goto bailout;
		}

		unit->Shared = shared;
		unit->Opens++;

		if ( ( unit->SerialSessions ) && ( unit->Opens == 1 ) )
		{
			IExec->Signal( unit->AbortPort->mp_SigTask, 1UL << unit->AbortPort->mp_SigBit );
		}

		if ( base->Type == PORT_SERIAL )
		{
			SerialDefaults( (PTR)request );

			if ( unit->SerialParamsValid )
			{
				CopyLineParams( (PTR)request, &unit->SerialParams );
			}
		}
	}

opened:
	base->Library.lib_OpenCnt++;
	request->io_Device = (PTR) base;
	request->io_Unit = (PTR) unit;
	request->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
	request->io_Error = 0;

bailout:
	if ( request->io_Error )
	{
		request->io_Device = NULL;
	}

	IExec->ReleaseSemaphore( & base->Lock );

	return ( request->io_Error );
}

// --
