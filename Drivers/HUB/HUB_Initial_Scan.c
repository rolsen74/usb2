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
	U32 scan_connected;

	scan_connected = ( in->HCDNode->hn_HCDType == HCDTYPE_EHCI );

	for ( port = 1; port <= in->Ports; port++ )
	{
		if ( in->PortWork )
		{
			// Give connected PCI EHCI hub ports their first attempt in port order.
			if ( ( scan_connected ) && ( HUB_Can_Enumerate( usbbase, in ) ) &&
				( HUB__Port_Status_Get( usbbase, in, port, &status ) == USB2Err_NoError ) &&
				( status.wPortStatus & PORTSTATF_Connection ) )
			{
				if ( status.wPortChange )
				{
					HUB__Port_Chg( usbbase, in, port );
				}

				if ( HUB__Port_Attach( usbbase, in, port ) == HUB_Attach_Complete )
				{
					in->PortWork[port].Due = 0;
					continue;
				}
			}

			in->PortWork[port].InitialSample = TRUE;

			HUB_Queue_Port( usbbase, in, port, TRUE );

			continue;
		}

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
