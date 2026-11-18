
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
UHCI_Chip_Alloc ( struct USB2_HCDNode * hn )
{
	struct USBBase * usbbase;

	S32 retval;

	usbbase = hn->hn_USBBase;
	retval = FALSE;

	TASK_NAME_ENTER ( "UHCI : UHCI_Chip_Alloc" );

	//-------------

	if ( ! TASK_ALLOCSIGNAL ( &hn->hn_HCD.UHCI.Signal_INT ) )
	{
		USBERROR ( "Error finding Signal" );
		goto bailout;
	}

	if ( ! TASK_ALLOCSIGNAL ( &hn->hn_HCD.UHCI.Signal_EI ) )
	{
		USBERROR ( "Error finding Signal" );
		goto bailout;
	}

	if ( ! TASK_ALLOCSIGNAL ( &hn->hn_HCD.UHCI.Signal_RD ) )
	{
		USBERROR ( "Error finding Signal" );
		goto bailout;
	}

	if ( ! TASK_ALLOCSIGNAL ( &hn->hn_HCD.UHCI.Signal_HSE ) )
	{
		USBERROR ( "Error finding Signal" );
		goto bailout;
	}

	if ( ! TASK_ALLOCSIGNAL ( &hn->hn_HCD.UHCI.Signal_HCPE ) )
	{
		USBERROR ( "Error finding Signal" );
		goto bailout;
	}

	if ( ! TASK_ALLOCSIGNAL ( &hn->hn_HCD.UHCI.Signal_HCH ) )
	{
		USBERROR ( "Error finding Signal" );
		goto bailout;
	}

	//-------------

	hn->HCD_Mask = 0;
	hn->HCD_Mask |= hn->hn_HCD.UHCI.Signal_INT.sig_Signal_Mask;
	hn->HCD_Mask |= hn->hn_HCD.UHCI.Signal_EI.sig_Signal_Mask;
	hn->HCD_Mask |= hn->hn_HCD.UHCI.Signal_RD.sig_Signal_Mask;
	hn->HCD_Mask |= hn->hn_HCD.UHCI.Signal_HSE.sig_Signal_Mask;
	hn->HCD_Mask |= hn->hn_HCD.UHCI.Signal_HCPE.sig_Signal_Mask;
	hn->HCD_Mask |= hn->hn_HCD.UHCI.Signal_HCH.sig_Signal_Mask;

	// --

	retval = TRUE;

bailout:

	TASK_NAME_LEAVE ();

	return ( retval );
}

// --
