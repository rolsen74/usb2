
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
UHCI_Isochronous_Length ( struct USB2_HCDNode * hn, struct RealRequest * ioreq )
{
	struct USBBase * usbbase;
	struct UHCI_QH * qh;
	struct UHCI_TD * td;

	U32 actual;
	U32 offset;
	U32 got;
	U32 status;

	usbbase = hn->hn_USBBase;
	qh		= ioreq->req_HCD.UHCI.QH_Header;
	actual	= 0;
	offset	= 0;

	if ( ! qh )
	{
		return ( 0 );
	}

	for ( td = qh->pqh_first_td; td; td = td->ptd_virt_next )
	{
		status = LE_SWAP32 ( td->td_status );

		if ( status & UHCI_TD_ACTIVE )
		{
			break;
		}

		got = ( status & UHCI_TD_ERRORS ) ? 0 : ( status + 1 ) & 0x7ff;
		got = MIN ( got, td->ptd_buffer_size );

		if ( ( ioreq->req_Public.io_Command == CMD_READ ) && ( got ) )
		{
			MEM_COPY ( td->ptd_copy_virt, (U8 *)ioreq->req_Public.io_Data + offset, got );
		}

		actual += got;
		offset += got;
	}

	return ( actual );
}

// --
