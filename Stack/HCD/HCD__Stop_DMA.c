
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static void
HCD_CloseSubmissions( struct USBBase *usbbase, struct USB2_HCDNode *hn, S32 recovering )
{
	SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

	if ( hn->hn_AcceptIO )
	{
		hn->hn_IOEpoch++;
	}

	hn->hn_AcceptIO = FALSE;
	hn->hn_RootHubRecovering = recovering && ( ( hn->hn_IsEHCI ) || ( hn->hn_IsOHCI ) || ( hn->hn_HCDType == HCDTYPE_UHCI ) || ( hn->hn_HCDType == HCDTYPE_XHCI ) );

	SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );
}

// --

SEC_CODE static S32
HCD_StopDMA( struct USB2_HCDNode *hn )
{
	if ( hn->hn_DMAActive )
	{
		if ( ( ! hn->HCD_Functions.Chip_Stop ) || ( ! hn->HCD_Functions.Chip_Stop( hn ) ) )
		{
			return ( FALSE );
		}

		hn->hn_DMAActive = FALSE;
	}

	return ( TRUE );
}

// --

SEC_CODE void
USB2_HCD_RequestReset( struct USB2_HCDNode *hn )
{
	struct USBBase *usbbase;

	usbbase = hn->hn_USBBase;

	HCD_CloseSubmissions( usbbase, hn, TRUE );

	TASK_SIGNAL( hn->hn_Task->tn_TaskAdr, hn->HCD_Restart_Chip.sig_Signal_Mask );
}

// --
