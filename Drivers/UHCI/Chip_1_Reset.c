
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

/*
** Copyright (c) 1998 The NetBSD Foundation, Inc.
** All rights reserved.
**
** This code is derived from software contributed to The NetBSD Foundation
** by Lennart Augustsson (lennart@augustsson.net) at
** Carlstedt Research & Technology.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
** ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
** TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
** PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
** BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

// --

#include "usb2_all.h"
#include "UHCI.h"

// --

/*
** Called before HCD PCI Interrupt is added
*/

SEC_CODE S32
UHCI_Chip_Reset ( struct USB2_HCDNode * hn )
{
	struct USBBase * usbbase;

	S32 retval;
	U32 cnt;

	usbbase = hn->hn_USBBase;
	retval = FALSE;

	TASK_NAME_ENTER ( "UHCI : UHCI_Chip_Reset" );

	PCI_WRITEWORD( UHCI_INTR, 0 );

	/* port resets */

	PCI_WRITEWORD ( UHCI_PORTSC ( 1 ), UHCI_PORTSC_PR );
	PCI_WRITEWORD ( UHCI_PORTSC ( 2 ), UHCI_PORTSC_PR );

	/* global reset */

	PCI_WRITEWORD ( UHCI_CMD, UHCI_CMD_GRESET );

	/* wait a little */

	HCD_WAIT_MS ( hn, USB_BUS_RESET_DELAY );

	/* do nothing */

	PCI_WRITEWORD ( UHCI_CMD, 0 );

	HCD_WAIT_MS ( hn, 10 ); // wait 10ms

	// Must be done before Setting the Interrupt Handler

	PCI_WRITEWORD ( UHCI_CMD, UHCI_CMD_HCRESET );

	/* The reset bit goes low when the controller is done. */
	for ( cnt = 0; cnt < UHCI_RESET_TIMEOUT; cnt++ )
	{
		if ( PCI_READWORD ( UHCI_CMD ) & UHCI_CMD_HCRESET )
		{
			HCD_WAIT_MS ( hn, 1 ); // wait 1ms
		}
		else
		{
			break;
		}
	}

	if ( cnt >= UHCI_RESET_TIMEOUT )
	{
		// SHOWMSG ( "Controller did not reset" );
		goto bailout;
	}

	/* Wait another 10 ms */

	HCD_WAIT_MS ( hn, 10 ); // wait 10ms

	PCI_WRITEWORD( UHCI_STS, UHCI_STS_ALLINTRS );

	hn->hn_HCD.UHCI.FatalStatus = 0;

	MEM_SET( hn->hn_HCD.UHCI.PortResetChange, 0, sizeof( hn->hn_HCD.UHCI.PortResetChange ) );

	MEM_SET( hn->hn_HCD.UHCI.PortSuspendChange, 0, sizeof( hn->hn_HCD.UHCI.PortSuspendChange ) );

	retval = TRUE;

bailout:

	TASK_NAME_LEAVE ();

	return ( retval );
}
