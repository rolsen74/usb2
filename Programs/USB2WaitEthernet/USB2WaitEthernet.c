
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/dos.h>
#include <proto/exec.h>
#include <interfaces/usb2eth.h>
#include <stdio.h>

// --

static const char version[] __attribute__((used)) =
	"\0$VER: USB2WaitEthernet 1.0 (22.11.2026)";

enum
{
	ARG_UNIT,
	ARG_TIMEOUT,
	ARG_RTL8152,
	ARG_COUNT
};

// --

static LONG
Wait_For_Adapter( ULONG unit, ULONG timeout, BOOL rtl8152 )
{
	struct USB2EthIFace *ethernet;
	struct IORequest request;

	ULONG type;
	ULONG polls;
	LONG result;
	BOOL opened;

	request = (struct IORequest) { 0 };
	request.io_Message.mn_Length = sizeof( request );
	ethernet = NULL;
	polls = 0;
	result = RETURN_WARN;
	opened = FALSE;

	while ( TRUE )
	{
		if ( IExec->SetSignal( 0, SIGBREAKF_CTRL_C ) & SIGBREAKF_CTRL_C )
		{
			break;
		}

		if ( ! opened )
		{
			request.io_Device = NULL;

			opened = ! IExec->OpenDevice( USB2_ETH_DEVICE, USB2_ETH_BACKEND_UNIT, & request, 0 );

			if ( ( opened ) && ( request.io_Device->dd_Library.lib_Revision >= USB2ETH_ADAPTER_REVISION ) )
			{
				ethernet = (APTR) IExec->GetInterface( (APTR) request.io_Device, "main", 1, NULL );
			}

			if ( ( opened ) && ( ! ethernet ) )
			{
				puts( "USB2WaitEthernet requires usb2ethernet.device with adapter lookup support." );

				result = RETURN_ERROR;
				break;
			}
		}

		if ( ethernet )
		{
			type = ethernet->GetAdapter( NULL, unit );

			if ( ( type != USB2ETH_ADAPTER_UNKNOWN ) && ( ( ! rtl8152 ) || ( type == USB2ETH_ADAPTER_RTL8152 ) ) )
			{
				result = RETURN_OK;
				break;
			}
		}

		if ( ! timeout )
		{
			printf( "USB2WaitEthernet: %s unit %lu did not become ready.\n",
				rtl8152 ? "RTL8152" : USB2_ETH_DEVICE, unit );

			break;
		}

		IDOS->Delay( TICKS_PER_SECOND / 5 );

		if ( ++polls == 5 )
		{
			polls = 0;
			timeout--;
		}
	}

	if ( ethernet )
	{
		IExec->DropInterface( (APTR) ethernet );
	}

	if ( opened )
	{
		IExec->CloseDevice( & request );
	}

	return ( result );
}

// --

int
main( void )
{
	struct RDArgs *args;

	LONG values[ARG_COUNT];
	LONG unit;
	LONG timeout;
	LONG result;

	values[ARG_UNIT] = 0;
	values[ARG_TIMEOUT] = 0;
	values[ARG_RTL8152] = 0;

	args = IDOS->ReadArgs( "UNIT/K/N,TIMEOUT/K/N,RTL8152/S", values, NULL );

	if ( ! args )
	{
		IDOS->PrintFault( IDOS->IoErr(), "USB2WaitEthernet" );

		return ( RETURN_ERROR );
	}

	unit = values[ARG_UNIT] ? *(LONG *) values[ARG_UNIT] : 0;
	timeout = values[ARG_TIMEOUT] ? *(LONG *) values[ARG_TIMEOUT] : 15;

	if ( ( unit < 0 ) || ( unit >= USB2_ETH_MAX_UNITS ) || ( timeout < 0 ) )
	{
		puts( "Use UNIT 0-31 and a TIMEOUT in seconds." );

		result = RETURN_ERROR;
	}
	else
	{
		result = Wait_For_Adapter( unit, timeout, values[ARG_RTL8152] != 0 );
	}

	IDOS->FreeArgs( args );

	return ( result );
}

// --
