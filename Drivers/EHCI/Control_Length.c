
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
#include "EHCI.h"

// --

/*
** Should set io_Error if there are transfer errors
*/

SEC_CODE U32
EHCI_Control_Length( struct USB2_HCDNode *hn, struct RealRequest *ioreq )
{
	struct USBBase *usbbase UNUSED;
	struct EHCI_TD *td;
	struct EHCI_QH *qh;

	U32 status;
	U32 actual;
	U32 remaining;
	U32 length;
	S32 short_packet;

	usbbase = hn->hn_USBBase;
	qh = ioreq->req_HCD.EHCI.QH_Header;
	actual = 0;
	short_packet = FALSE;

	TASK_NAME_ENTER( "EHCI : EHCI_Control_Length" );

	if ( ! qh )
	{
		ioreq->req_Public.io_Error = USB2Err_Host_HostError;
		goto bailout;
	}

	for ( td = qh->pqh_first_td ; td ; td = td->ptd_virt_next )
	{
		status = LE_SWAP32( td->td_status );

		if ( status & EHCI_TD_HALTED )
		{
			if ( status & EHCI_TD_BABBLE )
			{
				ioreq->req_Public.io_Error = USB2Err_Host_Overflow;
			}
			else if ( status & EHCI_TD_BUFERR )
			{
				ioreq->req_Public.io_Error = USB2Err_Host_UnknownError;
			}
			else if ( status & EHCI_TD_XACTERR )
			{
				ioreq->req_Public.io_Error = USB2Err_Host_Timeout;
			}
			else
			{
				ioreq->req_Public.io_Error = USB2Err_Host_Stall;
			}

			goto bailout;
		}

		if ( status & EHCI_TD_ACTIVE )
		{
			ioreq->req_Public.io_Error = USB2Err_Host_HostError;
			goto bailout;
		}

		if ( ( td == qh->pqh_first_td ) || ( ! td->ptd_buffer_virt ) )
		{
			continue;
		}

		remaining = EHCI_TD_GET_BYTES( status );

		if ( remaining > td->ptd_buffer_size )
		{
			ioreq->req_Public.io_Error = USB2Err_Host_HostError;
			goto bailout;
		}

		length = td->ptd_buffer_size - remaining;
		short_packet = remaining != 0;

		if ( ( length ) && ( ioreq->req_Public.io_Command == CMD_READ ) )
		{
			MEM_COPY( td->ptd_buffer_virt, (U8 *) ioreq->req_Public.io_Data + actual, length );
		}

		actual += length;
	}

	if ( ( short_packet ) && ( ! ioreq->req_Public.io_AllowShortPackets ) )
	{
		ioreq->req_Public.io_Error = USB2Err_Host_RuntPacket;
	}

bailout:

	TASK_NAME_LEAVE();

	return ( actual );
}

// --
