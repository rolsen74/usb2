
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
UHCI_Chip_Start( struct USB2_HCDNode *hn )
{
	struct USBBase *usbbase;

	U32 command;
	U32 status;
	U32 attempt;

	usbbase = hn->hn_USBBase;
	status = PCI_READWORD( UHCI_STS );

	if ( ( ! hn->hn_HCD.UHCI.FrameList_PhyAddr ) || ( hn->hn_HCD.UHCI.FatalStatus ) || ( status == 0xffff ) ||
		( ! ( status & UHCI_STS_HCH ) ) || ( status & ( UHCI_STS_HSE | UHCI_STS_HCPE ) ) )
	{
		return ( FALSE );
	}

	if ( hn->hn_HCD.UHCI.LegacySupport )
	{
		hn->hn_PCIDevice->WriteConfigWord( UHCI_LEGSUP, UHCI_LEGSUP_PIRQ | UHCI_LEGSUP_RWC );
	}

	__asm__ volatile( "sync" ::: "memory" );

	PCI_WRITELONG( UHCI_FLBASEADDR, hn->hn_HCD.UHCI.FrameList_PhyAddr );

	PCI_WRITEWORD( UHCI_INTR, UHCI_INTR_TOCRCIE | UHCI_INTR_RIE | UHCI_INTR_IOCE | UHCI_INTR_SPIE );

	command = UHCI_CMD_RS | UHCI_CMD_CF | UHCI_CMD_MAXP;

	PCI_WRITEWORD( UHCI_CMD, command );

	for( attempt = 0; attempt < 5; attempt++ )
	{
		status = PCI_READWORD( UHCI_STS );

		if ( ( hn->hn_HCD.UHCI.FatalStatus ) || ( status == 0xffff ) || ( status & ( UHCI_STS_HSE | UHCI_STS_HCPE ) ) )
		{
			return ( FALSE );
		}

		if ( ! ( status & UHCI_STS_HCH ) )
		{
			return ( TRUE );
		}

		HCD_WAIT_MS( hn, 1 );
	}

	return ( FALSE );
}

// --
