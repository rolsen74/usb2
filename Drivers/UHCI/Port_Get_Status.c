
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

SEC_CODE U32
UHCI_Port_Get_Status ( struct USB2_HCDNode * hn, U32 port )
{
	U32 status;
	U32 retval;

#if defined( DO_PANIC ) || defined( DO_ERROR ) || defined( DO_DEBUG ) || defined( DO_INFO )
	struct USBBase * usbbase;
	usbbase = hn->hn_USBBase;
#endif

	USBDEBUG ( "UHCI : PORT_Get_Status( Port %ld )", port );

	if ( ( port == 0 ) || ( hn->hn_HUB_NumPorts < port ) )
	{
		USBPANIC ( "Invalid Port Number #%lu", port );

		return ( 0 );
	}

	status = PCI_READWORD ( UHCI_PORTSC ( port ) );

	if ( status == 0xffff )
	{
		return ( 0 );
	}

	retval = ( 0
			   // Connect Status Change
			   | ( ( status & 0x0002 ) ? HUBF_Chg_Connection : 0 )
			   // Port Enable/Disable Change
			   | ( ( status & 0x0008 ) ? HUBF_Chg_Enable : 0 )
			   // Port Reset Change
			   | ( ( status & 0x0200 ) ? HUBF_Status_Reset : 0 )
			   | ( hn->hn_HCD.UHCI.PortResetChange[port] ? HUBF_Chg_Reset : 0 )
				   | ( hn->hn_HCD.UHCI.PortSuspendChange[port] ? HUBF_Chg_Suspend : 0 )

			   // Current Connect Status
			   | ( ( status & 0x0001 ) ? HUBF_Status_Connection : 0 )
			   // Port Enable/Disable Status
			   | ( ( status & 0x0004 ) ? HUBF_Status_Enable : 0 )
			   // Port LowSpeed Status
			   | ( ( status & 0x0100 ) ? HUBF_Status_Low_Speed : 0 )
			   // Port Suspend Status
			   | ( ( status & 0x1000 ) ? HUBF_Status_Suspend : 0 ) );

	// UHCI root ports are permanently powered.
	retval |= HUBF_Status_Power;

#ifdef DO_DEBUG
	USBDEBUG ( "UHCI : Port status %08lx, change %08lx", retval & 0x0000ffff, retval & 0xffff0000 );
#endif

	return ( retval );
}
