
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

SEC_CODE struct UHCI_TD *
UHCI_TD_Init ( struct USB2_HCDNode * hn, U32 pid, U32 length, U32 toggle, U32 speed, U32 address, U32 endpoint, U32 ioc,
		  U32 short_ok )
{
	struct UHCI_TD * td;

	U32 token;
	U32 status;

	td = UHCI_Get_TDBuffer ( hn );

	if ( ! td )
	{
		return ( NULL );
	}

	token  = pid | UHCI_TD_SET_DEVADDR ( address ) | UHCI_TD_SET_ENDPT ( endpoint ) | UHCI_TD_SET_DT ( toggle )
			 | ( ( length ? length - 1 : 0x7ff ) << 21 );
	status = UHCI_TD_ACTIVE | UHCI_TD_SET_ERRCNT ( 3 );

	if ( speed == USBSPEED_Low )
	{
		status |= UHCI_TD_LS;
	}

	if ( ioc )
	{
		status |= UHCI_TD_IOC;
	}

	if ( short_ok )
	{
		status |= UHCI_TD_SPD;
	}

	td->td_token  = LE_SWAP32 ( token );
	td->td_status = LE_SWAP32 ( status );

	return ( td );
}

// --
