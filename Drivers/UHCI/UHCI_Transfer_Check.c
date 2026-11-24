
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

SEC_CODE static S32
CheckIsochronous( struct USB2_HCDNode *hn, struct UHCI_QH *qh )
{
	struct UHCI_TD *td;

	U32 now;
	U32 elapsed;
	U32 status;
	S32 complete;

	now = PCI_READWORD( UHCI_FRNUM ) & UHCI_FRAME_MASK;
	complete = TRUE;

	__asm__ volatile( "sync" ::: "memory" );

	for( td = qh->pqh_first_td; td; td = td->ptd_virt_next )
	{
		status = LE_SWAP32( *(volatile U32 *)&td->td_status );

		if ( ( td->ptd_missed ) || ( ! ( status & UHCI_TD_ACTIVE ) ) )
		{
			continue;
		}

		elapsed = ( now - td->ptd_frame ) & UHCI_FRAME_MASK;

		if ( ( elapsed > 1 ) && ( elapsed <= UHCI_FRAMELIST_MASK ) )
		{
			UHCI_Isochronous_Unlink_TD( hn, td );

			if ( ! hn->hn_AcceptIO )
			{
				return ( FALSE );
			}

			td->ptd_missed = TRUE;
		}
		else
		{
			complete = FALSE;
		}
	}

	return ( complete );
}

// --

SEC_CODE S32
UHCI_Transfer_Check( struct USB2_HCDNode *hn, struct RealRequest *req )
{
	struct UHCI_QH *qh;
	struct UHCI_TD *td;

	U32 status;
	U32 actual;
	U32 pid;

	qh = req->req_HCD.UHCI.QH_Header;

	if ( ! qh )
	{
		return ( TRUE );
	}

	if ( qh->pqh_isoc )
	{
		return ( CheckIsochronous( hn, qh ) );
	}

	for( td = qh->pqh_first_td; td; td = td->ptd_virt_next )
	{
		if ( ( qh->pqh_short_advanced ) && ( td->ptd_is_data ) )
		{
			continue;
		}

		status = LE_SWAP32( *(volatile U32 *)&td->td_status );

		if ( status & UHCI_TD_ACTIVE )
		{
			return ( FALSE );
		}

		if ( status & UHCI_TD_ERRORS )
		{
			pid = LE_SWAP32( td->td_token ) & 0xff;

			if ( status & UHCI_TD_BABBLE )
			{
				req->req_Public.io_Error = USB2Err_Host_Overflow;
			}
			else if ( ( status & UHCI_TD_ERRORS ) == UHCI_TD_STALLED )
			{
				req->req_Public.io_Error = USB2Err_Host_Stall;
			}
			else if ( ( status & UHCI_TD_CRCTO ) && ( pid != UHCI_TD_PID_SETUP ) )
			{
				req->req_Public.io_Error = ( pid == UHCI_TD_PID_IN ) ? USB2Err_Host_CRCError : USB2Err_Host_Timeout;
			}
			else
			{
				req->req_Public.io_Error = USB2Err_Host_HostError;
			}

			return ( TRUE );
		}

		actual = ( status + 1 ) & 0x7ff;

		if ( ( td->ptd_is_data ) && ( req->req_Public.io_Command == CMD_READ ) &&
			( actual < td->ptd_buffer_size ) )
		{
			qh->pqh_runt = ! req->req_Public.io_AllowShortPackets;

			if ( req->req_EndPoint->ep_Type == EPATT_Type_Control )
			{
				// SPD leaves the hardware queue at the short descriptor.
				qh->pqh_short_advanced = TRUE;

				__asm__ volatile( "sync" ::: "memory" );

				qh->qh_elink = LE_SWAP32( qh->pqh_last_td->ptd_self_phy );

				return ( FALSE );
			}

			break;
		}
	}

	if ( qh->pqh_runt )
	{
		req->req_Public.io_Error = USB2Err_Host_RuntPacket;
	}

	return ( TRUE );
}

// --
