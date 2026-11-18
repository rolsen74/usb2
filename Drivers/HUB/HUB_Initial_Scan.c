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
HUB_Initial_Scan( struct USBBase *usbbase, struct intern *in )
{
	struct USB2_PortStatus status;

	U32 port;

	for ( port = 1; port <= in->Ports; port++ )
	{
		if ( HUB__Port_Status_Get( usbbase, in, port, &status ) != USB2Err_NoError )
		{
			continue;
		}

		if ( status.wPortChange )
		{
			HUB__Port_Chg( usbbase, in, port );
		}

		if ( ( status.wPortStatus & PORTSTATF_Connection ) &&
			( ! in->PortFunctions[port] ) )
		{
			HUB__Port_Function_Add( usbbase, in, port );
		}
	}
}

// --
