/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "XHCI.h"

// --

SEC_CODE U32
XHCI_Handler_Reset ( struct ExceptionContext * context UNUSED, struct ExecBase * sysbase UNUSED, PTR data )
{
	struct USB2_HCDNode * hn;
	struct XHCI_State * state;

	hn = data;
	state = hn->hn_HCD.XHCI.State;

	if ( state )
	{
		PCI_WRITELONG ( state->Operational + XHCI_USBCMD, 0 );
	}

	return ( FALSE );
}

// --
