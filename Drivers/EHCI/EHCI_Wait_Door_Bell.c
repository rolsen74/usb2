
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include "EHCI.h"

// --

SEC_CODE void EHCI_Wait_Door_Bell( struct USB2_HCDNode *hn )
{
U32 mask;
U32 val;
U32 adr;

	struct USBBase *usbbase = hn->hn_USBBase;
	TASK_NAME_ENTER( "EHCI : EHCI_Wait_Door_Bell" );

//	USBDEBUG( "EHCI_Wait_Door_Bell : Enter" );

	#ifdef DO_DEBUG

	// Make sure only one HCDs use this timer

	if ( hn->hn_Delay_InUse )
	{
		USBPANIC( "EHCI_Wait_Door_Bell : Error Delay in use" );		
	}

	hn->hn_Delay_InUse = TRUE;

	#endif

	// Set Wait Mask
	mask = hn->hn_HCD.EHCI.Signal_IAA.sig_Signal_Mask | hn->hn_Delay_MsgPort.ump_Signal.sig_Signal_Mask ;

	// Clear Signals
	TASK_SETSIGNAL( 0, mask );

	// Set Timer
	hn->hn_Delay_IOReq.Time.Microseconds	= 1000 * 100;
	hn->hn_Delay_IOReq.Time.Seconds			= 0;

	// Ask for doorbell
	adr	 = hn->hn_HCD.EHCI.CapLength + EHCI_USBCMD;
	val  = PCI_READLONG( adr );
	val	|= EHCI_CMD_IAAD;
	PCI_WRITELONG( adr, val );

	// Start Timer
	IO_SEND( & hn->hn_Delay_IOReq );

	// Wait for Timer or Bell

	#ifdef DO_DEBUG

	mask = TASK_WAIT( mask );

	IO_STOP( & hn->hn_Delay_IOReq );

	struct ExecIFace *IExec = hn->hn_USBBase->usb_IExec;

	/**/ if ( mask & hn->hn_HCD.EHCI.Signal_IAA.sig_Signal_Mask )
	{
//		IExec->DebugPrintF( "Got Door Bell\n" );
	}
	else if ( mask & hn->hn_Delay_MsgPort.ump_Signal.sig_Signal_Mask )
	{
		IExec->DebugPrintF( "Door Bell Timed Out\n" );
	}
	else
	{
		IExec->DebugPrintF( "Door Bell : Unknown Flags : $%08lx\n", mask );
	}

	// Debug only
	hn->hn_Delay_InUse = FALSE;

	#else

	TASK_WAIT( mask );

	IO_STOP( & hn->hn_Delay_IOReq );

	#endif

//	USBDEBUG( "EHCI_Wait_Door_Bell : Leave" );

	TASK_NAME_LEAVE();
}

// --
