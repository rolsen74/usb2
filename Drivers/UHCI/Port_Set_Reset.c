
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

SEC_CODE S32
UHCI_Port_Set_Reset ( struct USB2_HCDNode * hn, U32 port )
{
	struct USBBase * usbbase;

	S32 err;
	U32 val;

	usbbase = hn->hn_USBBase;

	USBDEBUG ( "UHCI_Port_Set_Reset( Port #%lu )", port );

	if ( ( port <= 0 ) || ( hn->hn_HUB_NumPorts < port ) )
	{
		USBDEBUG ( "Invalid Port Number #%lu", port );

		err = USB2Err_Host_HostError;
	}
	else
	{
		/* Start reset sequence. */

		val = PCI_READWORD ( UHCI_PORTSC ( port ) ) & 0xfff5;

		val |= UHCI_PORTSC_PR;

		PCI_WRITEWORD ( UHCI_PORTSC ( port ), val );

		HCD_WAIT_MS ( hn, 50 ); // Wait 50ms

		/* Clear port reset bit */

		val = PCI_READWORD ( UHCI_PORTSC ( port ) ) & 0xfdf5;

		PCI_WRITEWORD ( UHCI_PORTSC ( port ), val );

		HCD_WAIT_MS ( hn, 2 ); // Wait 2ms

		/* Enable port */

		val = PCI_READWORD ( UHCI_PORTSC ( port ) ) & 0xfff5;

		val |= UHCI_PORTSC_PE;

		PCI_WRITEWORD ( UHCI_PORTSC ( port ), val );

		HCD_WAIT_MS ( hn, 10 ); // Wait 10ms

		// Leave connection changes latched for the hub's post-reset status read.
		hn->hn_HCD.UHCI.PortResetChange[port] = 1;

		err = USB2Err_NoError;
	}

	return ( err );
}
