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
XHCI_Endpoint_Stop ( struct USB2_HCDNode * hn, struct RealRequest * req )
{
	struct XHCI_Slot * slot;
	struct XHCI_Ring * ring;
	struct USBBase * usbbase;

	U32 dci;
	U32 control;

	slot = req->req_HCD.XHCI.Slot;
	usbbase = hn->hn_USBBase;
	dci = req->req_HCD.XHCI.EndPoint;

	if ( ( ! slot ) || ( ! dci ) || ( req->req_HCD.XHCI.Finished ) )
	{
		return;
	}

	ring = &slot->EndPoints[dci];
	control = ( slot->Slot_ID << 24 ) | ( dci << 16 );

	if ( ! XHCI_Command ( hn, 0, 0, 0, XHCI_TRB_TYPE ( XHCI_TRB_STOP_EP ) | control, NULL ) )
	{
		XHCI_Chip_Stop ( hn );
		TASK_SIGNAL ( hn->hn_Task->tn_TaskAdr, hn->HCD_Restart_Chip.sig_Signal_Mask );

		return;
	}

	if ( ! XHCI_Command ( hn, ( ring->Physical + ring->Enqueue * sizeof ( struct XHCI_TRB ) ) | ring->Cycle,
		0, 0, XHCI_TRB_TYPE ( XHCI_TRB_SET_DEQUEUE ) | control, NULL ) )
	{
		XHCI_Chip_Stop ( hn );
		TASK_SIGNAL ( hn->hn_Task->tn_TaskAdr, hn->HCD_Restart_Chip.sig_Signal_Mask );
	}
}

// --
