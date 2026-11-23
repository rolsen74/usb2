
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

	S32 wait_error;
	U8 shared;
	U32 poll;
	U32 size;

	base = (PTR) Self->Data.LibBase;
	IExec = base->IExec;
	usbbase = base->USB;
	unit = NULL;
	request->io_Unit = NULL;
	request->io_Error = IOERR_OPENFAIL;

	// Class drivers use the backend unit to register without waiting.
	if ( ( number != USB2_PORT_BACKEND_UNIT ) && ( ! USB2_FullBoot_Wait( usbbase ) ) )
	{
		request->io_Device = NULL;

		return ( request->io_Error );
	}

	IExec->ObtainSemaphore( & base->Lock );

	if ( number != USB2_PORT_BACKEND_UNIT )
	{
		switch( base->Type )
		{
			case PORT_SERIAL: size = sizeof( struct IOExtSer ); break;
			case PORT_PARALLEL: size = sizeof( struct IOExtPar ); break;
			default: size = sizeof( struct IOStdReq ); break;
		}

		if ( request->io_Message.mn_Length < size )
		{
			if ( ( number == 0 ) && ( request->io_Message.mn_Length >= sizeof( struct IOStdReq ) ) )
			{
				goto opened;
			}

			goto bailout;
		}

		for ( poll = 0; ; poll++ )
		{
			unit = base->Units;

			while( ( unit ) && ( unit->Number < number ) )
			{
				unit = unit->Next;
			}

			if ( ( ( unit ) && ( unit->Number == number ) ) || ( poll == 30 ) )
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

		if ( ( ! unit ) || ( unit->Number != number ) )
		{
			unit = NULL;

			// Printer queries and writes both use IOStdReq.
			if ( ( number == 0 ) && ( base->Type == PORT_PRINTER ) )
			{
				goto opened;
			}

			goto bailout;
		}

		shared = SharedOpen( base, request );

		if ( ( unit->Closing ) || ( ( unit->Opens ) && ( ( ! unit->Shared ) || ( ! shared ) ) ) )
		{
			goto bailout;
		}

		unit->Shared = shared;
		unit->Opens++;

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
