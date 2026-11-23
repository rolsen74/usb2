
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

SEC_CODE void
UHCI_Bulk_Remove ( struct USB2_HCDNode * hn, struct RealRequest * req )
{
	struct UHCI_QH * qh;
	struct UHCI_QH * prev;
	struct UHCI_TD * td;

	U32 packets;
	U32 status;

	qh = req->req_HCD.UHCI.QH_Header;
	packets = 0;

	if ( ( ! qh ) || ( ! qh->pqh_linked ) )
	{
		return;
	}

	if ( ! UHCI_Chip_Stop ( hn ) )
	{
		USB2_HCD_RequestReset ( hn );

		return;
	}

	if ( req->req_Public.io_Error )
	{
		for ( td = qh->pqh_first_td; td; td = td->ptd_virt_next )
		{
			status = LE_SWAP32 ( td->td_status );

			if ( status & ( UHCI_TD_ACTIVE | UHCI_TD_ERRORS ) )
			{
				break;
			}

			if ( td->ptd_is_data )
			{
				packets++;
			}
		}

		req->req_EndPoint->ep_DataToggle ^= packets & 1;
	}

	prev = hn->hn_HCD.UHCI.Last_Start;

	while( ( prev ) && ( prev->pqh_virt_next != qh ) )
	{
		prev = prev->pqh_virt_next;
	}

	if ( ! prev )
	{
		USB2_HCD_RequestReset ( hn );

		return;
	}

	prev->qh_hlink = qh->qh_hlink;
	prev->pqh_virt_next = qh->pqh_virt_next;

	if ( hn->hn_HCD.UHCI.Last_End == qh )
	{
		hn->hn_HCD.UHCI.Last_End = prev;
	}

	qh->pqh_linked = FALSE;

	if ( ! UHCI_Chip_Start ( hn ) )
	{
		USB2_HCD_RequestReset ( hn );
	}
}

// --
