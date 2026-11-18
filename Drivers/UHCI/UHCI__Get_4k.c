
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
UHCI_Get_4kBuffer ( struct USB2_HCDNode * hn, struct UHCI_TD * td, PTR data, U32 len )
{
	struct USBBase * usbbase;

	S32	 retval;
	U32	 phy;
	U8 * mem;

	usbbase = hn->hn_USBBase;
	retval = FALSE;

	TASK_NAME_ENTER ( "UHCI : UHCI_Get_4kBuffer" );

	// --

	mem = MEMORY_ALLOC ( MEMID_HCD_4k, FALSE );

	if ( ! mem )
	{
		USBERROR ( "UHCI_Get_4kBuffer : Error allocating memory" );
		goto bailout;
	}

	phy = ( (struct Mem_FreeNode *)mem )->mfn_Addr;

#ifdef DO_PANIC

	if ( phy == 0 )
	{
		USBPANIC ( "Error PhyAddress is Zero (4k %p)", mem );
	}

	if ( phy & 0xf )
	{
		USBPANIC ( "Bad Alignment 0x%08lx", phy );
	}

	if ( len > 1 * 4096 )
	{
		USBPANIC ( "Buffer overflow ( len : %ld )", len );
	}

#endif

	/*
	** Init Transfer Descriptor.
	*/

	if ( td )
	{
		td->td_buffer		= LE_SWAP32 ( phy );
		td->ptd_buffer_virt = mem;
		td->ptd_buffer_phy	= phy;
		td->ptd_buffer_size = len;
	}

	if ( len )
	{
		if ( data )
		{
			// Send command
			// copy src, dest, len
			MEM_COPY ( data, mem, len );
		}
#ifdef DO_DEBUG
		else
		{
			MEM_SET ( mem, 0xac, 1 * 4096 );
		}
#endif
	}

	retval = TRUE;

bailout:

	TASK_NAME_LEAVE ();

	return ( retval );
}

// --
