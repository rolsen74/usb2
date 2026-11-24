/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "HUB.h"

// --

#define HUB_SAMPLE_US 25000U
#define HUB_STABLE_US 100000U
#define HUB_DEBOUNCE_US 500000U
#define HUB_RETRY_US 250000U
#define HUB_EMPTY_PORT_US 100000U
#define HUB_INITIAL_EMPTY_POLLS 15U
#define HUB_TRIES 5U

// --

SEC_CODE void
HUB_Queue_Port( struct USBBase *usbbase, struct intern *in, U32 port, U32 changed )
{
	struct HUB_PortWork *work;

	U64 now;

	work = &in->PortWork[port];
	now = HUB_Uptime( usbbase );

	if ( changed )
	{
		work->StableSince = 0;

		if ( ! work->Due )
		{
			work->DebounceUntil = now + HUB_DEBOUNCE_US;
			work->Attempts = 0;
		}
	}

	if ( ( ! work->Due ) || ( changed ) )
	{
		work->Due = now + 1;
	}
}

// --

SEC_CODE static void
RetryPort( struct USBBase *usbbase, struct intern *in, U32 port, U64 now )
{
	struct HUB_PortWork *work;

	work = &in->PortWork[port];
	work->Attempts++;
	work->StableSince = 0;
	work->DebounceUntil = now + HUB_RETRY_US + HUB_DEBOUNCE_US;
	work->Due = ( work->Attempts < HUB_TRIES ) ? now + HUB_RETRY_US : 0;

	if ( ! work->Due )
	{
		usbbase->usb_IExec->DebugPrintF( "USB HUB: HCD %lu hub %lu port %lu exhausted attach retries\n",
			in->HCDNode->hn_HCDIndex, in->Function->fkt_Address, port );
	}
}

// --

SEC_CODE static void
PollEmptyPort( struct HUB_PortWork *work, U64 now )
{
	work->StableSince = 0;

	if ( work->EmptyPolls )
	{
		work->EmptyPolls--;
		work->Due = now + HUB_EMPTY_PORT_US;
		work->DebounceUntil = 0;
	}
	else
	{
		work->Due = 0;
	}
}

// --

SEC_CODE static U32
PortReady( struct USBBase *usbbase, struct intern *in, struct HUB_PortWork *work, U32 port, U64 now )
{
	struct USB2_PortStatus status;

	if ( in->PortFunctions[port] )
	{
		work->Due = 0;

		return ( FALSE );
	}

	if ( ( ! work->Due ) || ( work->Due > now ) )
	{
		return ( FALSE );
	}

	if ( ( ! HUB_Can_Enumerate( usbbase, in ) ) || ( HUB__Port_Probe_Holds( usbbase, in, port ) ) )
	{
		work->Due = now + HUB_RETRY_US;

		return ( FALSE );
	}

	if ( HUB__Port_Status_Get( usbbase, in, port, &status ) != USB2Err_NoError )
	{
		RetryPort( usbbase, in, port, now );

		return ( FALSE );
	}

	now = HUB_Uptime( usbbase );

	// Count this port's actual samples so a slow sibling cannot exhaust its startup scan.
	if ( work->InitialSample )
	{
		work->InitialSample = FALSE;
		work->EmptyPolls = HUB_INITIAL_EMPTY_POLLS;
		work->DebounceUntil = now + HUB_DEBOUNCE_US;
	}

	if ( status.wPortChange & PORTCHGF_Connection )
	{
		// Count a flapping port against its current debounce window.
		if ( HUB__Port_Feature_Clr( usbbase, in, port, PORT_CLR_CONNECTION ) != USB2Err_NoError )
		{
			RetryPort( usbbase, in, port, now );

			return ( FALSE );
		}

		work->StableSince = 0;
	}

	if ( ! ( status.wPortStatus & PORTSTATF_Connection ) )
	{
		PollEmptyPort( work, now );

		return ( FALSE );
	}

	if ( ! work->DebounceUntil )
	{
		work->DebounceUntil = now + HUB_DEBOUNCE_US;
	}

	if ( ! ( status.wPortChange & PORTCHGF_Connection ) )
	{
		if ( ! work->StableSince )
		{
			work->StableSince = now;
		}
		else if ( now - work->StableSince >= HUB_STABLE_US )
		{
			return ( TRUE );
		}
	}

	if ( now >= work->DebounceUntil )
	{
		RetryPort( usbbase, in, port, now );
	}
	else
	{
		work->Due = now + HUB_SAMPLE_US;
	}

	return ( FALSE );
}

// --

SEC_CODE void
HUB_Port_Work( struct USBBase *usbbase, struct intern *in )
{
	struct HUB_PortWork *work;

	U32 port;
	U32 count;
	U32 ready;
	U64 now;

	now = HUB_Uptime( usbbase );
	ready = 0;
	port = in->NextPort;

	for ( count = 0; count < in->Ports; count++ )
	{
		port = ( port == in->Ports ) ? 1 : port + 1;
		work = &in->PortWork[port];

		if ( ( PortReady( usbbase, in, work, port, now ) ) && ( ! ready ) )
		{
			ready = port;
		}
	}

	if ( ! ready )
	{
		return;
	}

	in->NextPort = ready;
	work = &in->PortWork[ready];

	switch ( HUB__Port_Attach( usbbase, in, ready ) )
	{
		case HUB_Attach_Complete:
			work->Due = 0;
			work->EmptyPolls = 0;
			break;

		case HUB_Attach_Gone:
			now = HUB_Uptime( usbbase );

			PollEmptyPort( work, now );
			break;

		case HUB_Attach_Deferred:
			work->Due = HUB_Uptime( usbbase ) + HUB_SAMPLE_US;
			break;

		case HUB_Attach_Reconnect:
			work->Due = 0;

			HUB_Queue_Port( usbbase, in, ready, TRUE );
			break;

		default:
			RetryPort( usbbase, in, ready, HUB_Uptime( usbbase ) );
			break;
	}
}

// --
