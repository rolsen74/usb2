
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

/*
** Should set io_Error if there are transfer errors
*/

SEC_CODE U32
OHCI_Control_Length( struct USB2_HCDNode *hn, struct RealRequest *ioreq )
{
	struct USBBase *usbbase UNUSED;
	struct OHCI_ED *ed;
	struct OHCI_TD *td;

	U32 status;
	U32 actual;
	U32 remaining;
	U32 length;
	U32 cbp;
	U32 end;
	S32 error;

	usbbase = hn->hn_USBBase;
	ed = ioreq->req_HCD.OHCI.ED_Header;
	actual = 0;

	TASK_NAME_ENTER( "OHCI : OHCI_Control_Length" );

	if ( ed )
	{
		// The dummy tail is never executed.
		for ( td = ed->ped_first_td ; ( td ) && ( td->ptd_virt_next ) ; td = td->ptd_virt_next )
		{
			status = OHCI_TD_GET_CC( LE_SWAP32( td->td_flags ) );

			switch ( status )
			{
				case OHCI_CC_NO_ERROR:
					error = USB2Err_NoError;
					break;
				case OHCI_CC_STALL:
					error = USB2Err_Host_Stall;
					break;
				case OHCI_CC_CRC:
				case OHCI_CC_BIT_STUFFING:
				case OHCI_CC_PID_CHECK_FAILURE:
					error = USB2Err_Host_CRCError;
					break;
				case OHCI_CC_DEVICE_NOT_RESPONDING:
					error = USB2Err_Host_Timeout;
					break;
				case OHCI_CC_DATA_OVERRUN:
				case OHCI_CC_BUFFER_OVERRUN:
					error = USB2Err_Host_Overflow;
					break;
				case OHCI_CC_DATA_UNDERRUN:
					error = ( ( td != ed->ped_first_td ) && ( td->ptd_buffer_virt ) &&
						( ioreq->req_Public.io_Command == CMD_READ ) && ( ioreq->req_Public.io_AllowShortPackets ) )
						? USB2Err_NoError : USB2Err_Host_RuntPacket;
					break;
				case OHCI_CC_BUFFER_UNDERRUN:
					error = USB2Err_Host_RuntPacket;
					break;
				case OHCI_CC_UNEXPECTED_PID:
				case OHCI_CC_DATA_TOGGLE_MISMATCH:
				case OHCI_CC_NOT_ACCESSED:
					error = USB2Err_Host_HostError;
					break;
				default:
					error = USB2Err_Host_UnknownError;
					break;
			}

			if ( ( status & OHCI_CC_NOT_ACCESSED_MASK ) == OHCI_CC_NOT_ACCESSED )
			{
				ioreq->req_Public.io_Error = USB2Err_Host_HostError;
				break;
			}

			if ( ( td == ed->ped_first_td ) || ( ! td->ptd_buffer_virt ) )
			{
				if ( error )
				{
					ioreq->req_Public.io_Error = error;
					break;
				}

				continue;
			}

			cbp = LE_SWAP32( td->td_cbp );
			end = LE_SWAP32( td->td_be );
			remaining = ( cbp && ( end >= cbp ) ) ? end - cbp + 1 : 0;

			if ( remaining > td->ptd_buffer_size )
			{
				ioreq->req_Public.io_Error = USB2Err_Host_HostError;
				break;
			}

			length = td->ptd_buffer_size - remaining;

			if ( ( length ) && ( ioreq->req_Public.io_Command == CMD_READ ) )
			{
				MEM_COPY( td->ptd_buffer_virt, (U8 *) ioreq->req_Public.io_Data + actual, length );
			}

			actual += length;

			if ( error )
			{
				ioreq->req_Public.io_Error = error;
				break;
			}
		}
	}
	else
	{
		ioreq->req_Public.io_Error = USB2Err_Host_HostError;
	}

	TASK_NAME_LEAVE();

	return ( actual );
}

// --
