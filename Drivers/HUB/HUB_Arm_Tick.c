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

SEC_CODE void
HUB_Arm_Tick( struct USBBase *usbbase, struct intern *in )
{
	U64 next;
	U64 now;
	U64 delay;
	U32 port;

	next = in->HousekeepingDue;

	if ( in->PortWork )
	{
		for ( port = 1; port <= in->Ports; port++ )
		{
			if ( ( in->PortWork[port].Due ) && ( in->PortWork[port].Due < next ) )
			{
				next = in->PortWork[port].Due;
			}
		}
	}

	if ( in->Tick_TimerAdded )
	{
		if ( in->TickDue <= next )
		{
			return;
		}

		IO_STOP( &in->Tick_TimeRequest );
	}

	now = HUB_Uptime( usbbase );
	delay = ( next > now ) ? next - now : 1;
	in->Tick_TimeRequest.Time.Seconds = delay / 1000000;
	in->Tick_TimeRequest.Time.Microseconds = delay % 1000000;
	in->TickDue = next;
	in->Tick_TimerAdded = TRUE;

	IO_SEND( &in->Tick_TimeRequest );
}

// --
