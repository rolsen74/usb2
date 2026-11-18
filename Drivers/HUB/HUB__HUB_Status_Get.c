
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// --

#include "usb2_all.h"

#include "HUB.h"

// --

SEC_CODE S32 HUB__HUB_Status_Get( struct USBBase *usbbase, struct intern *in, struct USB2_HUBStatus *stat )
{
	struct USB2_IORequest *ioreq;
	struct USB2_SetupData *sd;

	ioreq = in->Res_Control->IORequests[0];
	sd = in->Res_Control->SetupData;

	sd->RequestType = REQTYPE_Read | REQTYPE_Class | REQTYPE_Device;
	sd->RequestCode = REQCODE_Get_Status;
	sd->Value = 0;
	sd->Index = 0;
	sd->Length = LE_SWAP16( sizeof( *stat ) );

	ioreq->io_Command = CMD_READ;
	ioreq->io_Data = stat;
	ioreq->io_Length = sizeof( *stat );
	ioreq->io_SetupData = sd;
	ioreq->io_SetupLength = sizeof( *sd );

	IO_DO( ioreq );

	if ( ioreq->io_Error != USB2Err_NoError )
	{
		return ( ioreq->io_Error );
	}

	if ( ioreq->io_Actual != sizeof( *stat ) )
	{
		return ( USB2Err_Host_RuntPacket );
	}

	stat->wHubStatus = LE_SWAP16( stat->wHubStatus );
	stat->wHubChange = LE_SWAP16( stat->wHubChange );

	return ( USB2Err_NoError );
}

// --
