
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

/*
** Copyright (c) 2001 The NetBSD Foundation, Inc.
** All rights reserved.
**
** This code is derived from software contributed to The NetBSD Foundation
** by Lennart Augustsson (lennart@augustsson.net).
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. All advertising materials mentioning features or use of this software
**    must display the following acknowledgement:
**        This product includes software developed by the NetBSD
**        Foundation, Inc. and its contributors.
** 4. Neither the name of The NetBSD Foundation nor the names of its
**    contributors may be used to endorse or promote products derived
**    from this software without specific prior written permission.
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
#include "OHCI.h"
 
// --

SEC_CODE S32
OHCI_Port_Set_Reset( struct USB2_HCDNode *hn, U32 port )
{
	struct USBBase *usbbase;

	U32 reg;
	U32 status;
	U32 pulse;
	U32 poll;
	U16 frame;

	usbbase = hn->hn_USBBase;

	if ( ( ! port ) || ( port > hn->hn_HUB_NumPorts ) )
	{
		return ( USB2Err_Host_HostError );
	}

	reg = OHCI_RH_PORT_STATUS( port );

	// OHCI generates 10 ms per command. Keep gaps below 3 ms
	// and leave the last reset-change bit for the hub worker.
	for ( pulse = 0 ; pulse < 5 ; pulse++ )
	{
		status = PCI_READLONG( reg );

		if ( ( ! ( status & OHCI_PORT_CCS ) ) || ( status & OHCI_PORT_CSC ) )
		{
			return ( USB2Err_Device_Detached );
		}

		PCI_WRITELONG( reg, OHCI_PORT_PRSC );

		frame = PCI_READLONG( OHCI_FM_NUMBER );

		PCI_WRITELONG( reg, OHCI_PORT_PRS );

		HCD_WAIT_MS( hn, 8 );

		for ( poll = 0 ; poll < 24 ; poll++ )
		{
			status = PCI_READLONG( reg );

			if ( ( ! ( status & OHCI_PORT_CCS ) ) || ( status & OHCI_PORT_CSC ) )
			{
				return ( USB2Err_Device_Detached );
			}

			if ( ( status & OHCI_PORT_PRSC ) && ( ! ( status & OHCI_PORT_PRS ) ) )
			{
				break;
			}

			MISC_MICRODELAY( 250 );
		}

		if ( ( poll == 24 ) || ( ( pulse < 4 ) && ( (U16) ( PCI_READLONG( OHCI_FM_NUMBER ) - frame ) >= 13 ) ) )
		{
			return ( USB2Err_Host_Timeout );
		}
	}

	return ( ( status & OHCI_PORT_PES ) ? USB2Err_NoError : USB2Err_Host_HostError );
}

// --
