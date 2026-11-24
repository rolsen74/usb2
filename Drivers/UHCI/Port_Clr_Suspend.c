
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
UHCI_Port_Clr_Suspend( struct USB2_HCDNode *hn, U32 port )
{
	struct USBBase *usbbase;

	U32 status;
	U32 poll;

	usbbase = hn->hn_USBBase;

	USBDEBUG( "UHCI_Port_Clr_Suspend( Port #%lu )", port );

	if ( ( ! port ) || ( port > hn->hn_HUB_NumPorts ) )
	{
		return ( USB2Err_Host_HostError );
	}

	status = PCI_READWORD( UHCI_PORTSC( port ) );

	if ( status == 0xffff )
	{
		return ( USB2Err_Device_Detached );
	}

	if ( ! ( status & UHCI_PORTSC_SUSP ) )
	{
		return ( USB2Err_NoError );
	}

	if ( ( status & ( UHCI_PORTSC_CCS | UHCI_PORTSC_PE ) ) != ( UHCI_PORTSC_CCS | UHCI_PORTSC_PE ) )
	{
		return ( USB2Err_Device_Detached );
	}

	PCI_WRITEWORD( UHCI_PORTSC( port ), UHCI_PORTSC_PE | UHCI_PORTSC_SUSP | UHCI_PORTSC_RD );

	HCD_WAIT_MS( hn, 20 );

	status = PCI_READWORD( UHCI_PORTSC( port ) );

	if ( ( status == 0xffff ) || ( ( status & ( UHCI_PORTSC_CCS | UHCI_PORTSC_CSC ) ) != UHCI_PORTSC_CCS ) )
	{
		PCI_WRITEWORD( UHCI_PORTSC( port ), 0 );

		return ( USB2Err_Device_Detached );
	}

	if ( ( status & ( UHCI_PORTSC_PE | UHCI_PORTSC_RD | UHCI_PORTSC_SUSP ) ) !=
		( UHCI_PORTSC_PE | UHCI_PORTSC_RD | UHCI_PORTSC_SUSP ) )
	{
		return ( USB2Err_Host_Timeout );
	}

	// Clear resume and suspend together to generate the low-speed EOP.
	PCI_WRITEWORD( UHCI_PORTSC( port ), UHCI_PORTSC_PE );

	for( poll = 0; poll <= 2; poll++ )
	{
		status = PCI_READWORD( UHCI_PORTSC( port ) );

		if ( ! ( status & ( UHCI_PORTSC_SUSP | UHCI_PORTSC_RD ) ) )
		{
			break;
		}

		if ( poll < 2 )
		{
			HCD_WAIT_MS( hn, 1 );
		}
	}

	if ( status & ( UHCI_PORTSC_SUSP | UHCI_PORTSC_RD ) )
	{
		return ( USB2Err_Host_Timeout );
	}

	HCD_WAIT_MS( hn, 10 );

	status = PCI_READWORD( UHCI_PORTSC( port ) );

	if ( ( status & ( UHCI_PORTSC_CCS | UHCI_PORTSC_CSC | UHCI_PORTSC_PE ) ) != ( UHCI_PORTSC_CCS | UHCI_PORTSC_PE ) )
	{
		return ( USB2Err_Device_Detached );
	}

	hn->hn_HCD.UHCI.PortSuspendChange[port] = TRUE;

	TASK_SIGNAL( hn->hn_Task->tn_TaskAdr, hn->hn_HUB_Signal.sig_Signal_Mask );

	return ( USB2Err_NoError );
}

// --
