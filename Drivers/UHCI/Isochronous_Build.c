
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
UHCI_Isochronous_Build( struct USB2_HCDNode *hn, struct RealRequest *ioreq )
{
struct USBBase *usbbase;
struct USB2_EndPointNode *ep;
struct RealFunctionNode *fn;
struct UHCI_QH *qh;
struct UHCI_TD *td;
struct UHCI_TD *last;

U8 *data;
U8 *chunk;
U32 left;
U32 offset;
U32 chunk_used;
U32 chunk_phy;
U32 len;
U32 pid;
S32 err;

usbbase = hn->hn_USBBase;
ep = ioreq->req_EndPoint;
fn = ioreq->req_Function;
last = NULL;
data = ioreq->req_Public.io_Data;
chunk = NULL;
left = ioreq->req_Public.io_Length;
offset = 0;
chunk_used = 0;
chunk_phy = 0;
err = USB2Err_Stack_NoMemory;

	if ( ( fn->fkt_Speed != USBSPEED_Full ) ||
		( ! ep->ep_MaxPacketSize ) ||
		( ep->ep_MaxPacketSize > 1023 ) ||
		( ! left ) ||
		( ! data ) ||
		( left > ep->ep_MaxPacketSize * ( UHCI_FRAMELIST_COUNT - 2 ) ) ||
		( ( ioreq->req_Public.io_IsoPacketLengths ) &&
		  ( ( ep->ep_Interval != 1 ) ||
		    ( ioreq->req_Public.io_IsoPacketCount != ( left + ep->ep_MaxPacketSize - 1 ) / ep->ep_MaxPacketSize ) ) ) )
	{
		err = USB2Err_Stack_InvalidStructure;
		goto failed;
	}

	qh = UHCI_Get_QHBuffer( hn, ioreq );

	if ( ! qh )
	{
		goto failed;
	}

	pid = ( ioreq->req_Public.io_Command == CMD_READ ) ? UHCI_TD_PID_IN : UHCI_TD_PID_OUT;

	while( left )
	{
		len = MIN( left, ep->ep_MaxPacketSize );

		if ( ( chunk_used + len ) > 4096 )
		{
			chunk = NULL;
			chunk_used = 0;
		}

		td = UHCI_TD_Init( hn, pid, len, 0, fn->fkt_Speed,
			fn->fkt_Address, ep->ep_Number, left == len, 0 );

		if ( ! td )
		{
			goto failed;
		}

		UHCI_TD_Append( qh, &last, td );
		/* Isochronous TDs are scheduled directly in frame list entries. */
		td->td_status = LE_SWAP32( UHCI_TD_ACTIVE | UHCI_TD_IOS |
			(( left == len ) ? UHCI_TD_IOC : 0) );

		if ( ! chunk )
		{
			chunk = MEMORY_ALLOC( MEMID_HCD_4k, FALSE );

			if ( ! chunk )
			{
				goto failed;
			}

			chunk_phy = ((struct Mem_FreeNode *)chunk)->mfn_Addr;
			td->ptd_buffer_virt = chunk;
			td->ptd_buffer_phy = chunk_phy;
		}

		td->ptd_copy_virt = chunk + chunk_used;
		td->ptd_buffer_size = len;
		td->ptd_is_data = TRUE;
		td->td_buffer = LE_SWAP32( chunk_phy + chunk_used );

		if ( pid == UHCI_TD_PID_OUT )
		{
			MEM_COPY( data + offset, chunk + chunk_used, len );
		}

		chunk_used += len;
		offset += len;
		left -= len;
	}

	return ( FALSE );

failed:

	if ( ! ioreq->req_Public.io_Error )
	{
		ioreq->req_Public.io_Error = err;
	}

	UHCI_Transfer_Free( hn, ioreq );

	return ( TRUE );
}

// --
