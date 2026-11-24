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

SEC_CODE void
XHCI_Chip_Dealloc ( struct USB2_HCDNode * hn )
{
	struct USBBase * usbbase;
	struct XHCI_State * state;

	usbbase = hn->hn_USBBase;
	state = hn->hn_HCD.XHCI.State;

	if ( ! state )
	{
		return;
	}

	if ( state->Legacy_Claimed )
	{
		PCI_WRITEBYTE ( state->Legacy_Offset + 3, 0 );
	}

	MSGPORT_FREE ( &hn->hn_TTClearPort );

	TASK_FREESIGNAL ( &state->Signal_Event );
	MEM_FREEVEC ( state );
	hn->hn_HCD.XHCI.State = NULL;
	hn->HCD_Mask = 0;
}

// --
