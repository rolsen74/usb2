
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
UHCI_Transfer_Check ( struct USB2_HCDNode * hn UNUSED, struct RealRequest * req )
{
	struct UHCI_QH * qh;
	struct UHCI_TD * td;
	struct UHCI_TD * last;

	U32				 status;
	U32				 wanted;
	U32				 actual;

	qh	 = req->req_HCD.UHCI.QH_Header;
	last = NULL;

	if ( ! qh )
	{
		return ( TRUE );
	}

	for ( td = qh->pqh_first_td; td; td = td->ptd_virt_next )
	{
		last = td;
	}

	for ( td = qh->pqh_first_td; td; td = td->ptd_virt_next )
	{
		if ( ( qh->pqh_short_advanced ) && ( td->ptd_is_data ) )
		{
			continue;
		}

		status = LE_SWAP32 ( td->td_status );

		if ( status & UHCI_TD_ACTIVE )
		{
			return ( FALSE );
		}

		if ( status & UHCI_TD_ERRORS )
		{
			req->req_Public.io_Error = ( status & UHCI_TD_STALLED ) ? USB2Err_Host_Stall : USB2Err_Host_HostError;

			return ( TRUE );
		}

		if ( ( req->req_EndPoint->ep_Type != EPATT_Type_Isochronous ) && ( td->ptd_is_data )
			 && ( req->req_Public.io_Command == CMD_READ ) )
		{
			wanted = td->ptd_buffer_size;
			actual = ( status + 1 ) & 0x7ff;

			if ( actual < wanted )
			{
				if ( ! req->req_Public.io_AllowShortPackets )
				{
					req->req_Public.io_Error = USB2Err_Host_RuntPacket;
				}

				if ( ( req->req_EndPoint->ep_Type == EPATT_Type_Control ) && ( last != td ) && ( ! qh->pqh_short_advanced )
					 && ( ! req->req_Public.io_Error ) )
				{
					qh->pqh_short_advanced = TRUE;
					qh->qh_elink		   = LE_SWAP32 ( last->ptd_self_phy );

					return ( FALSE );
				}

				return ( TRUE );
			}
		}
	}

	return ( TRUE );
}

// --
