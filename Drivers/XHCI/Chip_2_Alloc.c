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

SEC_CODE S32
XHCI_Chip_Alloc ( struct USB2_HCDNode * hn )
{
	struct USBBase * usbbase;
	struct XHCI_State * state;

	usbbase = hn->hn_USBBase;
	state = hn->hn_HCD.XHCI.State;

	if ( ( ! TASK_ALLOCSIGNAL ( &state->Signal_Event ) ) || ( ! MSGPORT_INIT ( &hn->hn_TTClearPort ) ) )
	{
		return ( FALSE );
	}

	hn->HCD_Mask = state->Signal_Event.sig_Signal_Mask | hn->hn_TTClearPort.ump_Signal.sig_Signal_Mask;

	return ( TRUE );
}

// --
