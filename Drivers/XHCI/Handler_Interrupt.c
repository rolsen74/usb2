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
XHCI_Handler_Interrupt ( struct ExceptionContext * context UNUSED, struct ExecBase * sysbase UNUSED, PTR data )
{
	struct USB2_HCDNode * hn;
	struct USBBase * usbbase;
	struct XHCI_State * state;

	U32 status;
	U32 mask;

	hn = data;
	usbbase = hn->hn_USBBase;
	state = hn->hn_HCD.XHCI.State;
	status = PCI_READLONG ( state->Operational + XHCI_USBSTS );

	if ( ! ( status & ( XHCI_STS_EVENT | XHCI_STS_PORT_CHANGE | XHCI_STS_HOST_ERROR ) ) )
	{
		return ( FALSE );
	}

	PCI_WRITELONG ( state->Operational + XHCI_USBSTS, status & ( XHCI_STS_EVENT | XHCI_STS_PORT_CHANGE | XHCI_STS_HOST_ERROR ) );
	PCI_WRITELONG ( state->Runtime + XHCI_IMAN, XHCI_IMAN_PENDING | XHCI_IMAN_ENABLE );
	mask = state->Signal_Event.sig_Signal_Mask;

	if ( status & XHCI_STS_HOST_ERROR )
	{
		mask |= hn->HCD_Restart_Chip.sig_Signal_Mask;
	}

	if ( status & XHCI_STS_PORT_CHANGE )
	{
		mask |= hn->hn_HUB_Signal.sig_Signal_Mask;
	}

	TASK_SIGNAL ( hn->hn_Task->tn_TaskAdr, mask );

	return ( FALSE );
}

// --
