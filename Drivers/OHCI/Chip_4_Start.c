
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

SEC_CODE S32 OHCI_Chip_Start( struct USB2_HCDNode *hn )
{
S32 retval;
U32 portnr;
U32 ival;
U32 fm;

	struct USBBase *usbbase UNUSED = hn->hn_USBBase;
	TASK_NAME_ENTER( "OHCI : OHCI_Chip_Start" );

	retval = FALSE;

	// --

	/*------------*
	 * OHCI Setup *
	 *------------*/

	if ( ( ! hn->hn_HCD.OHCI.HCCA_Table ) || ( ! hn->hn_HCD.OHCI.Ctrl_Head ) ||
		( ! hn->hn_HCD.OHCI.Bulk_Head ) )
	{
		goto bailout;
	}

	__asm__ volatile ( "sync" ::: "memory" );

	PCI_WRITELONG( OHCI_HCCA_PTR, hn->hn_HCD.OHCI.HCCA_Physical );

	PCI_WRITELONG( OHCI_CONTROL_HEAD_ED,	hn->hn_HCD.OHCI.Ctrl_Head->ped_self_phy );

	PCI_WRITELONG( OHCI_BULK_HEAD_ED,		hn->hn_HCD.OHCI.Bulk_Head->ped_self_phy );

	PCI_WRITELONG( OHCI_INTERRUPT_DISABLE,	OHCI_ALL_INTRS );

	PCI_WRITELONG( OHCI_CONTROL,			OHCI_PLE | OHCI_IE | OHCI_CLE | OHCI_BLE | OHCI_RATIO_1_4 | OHCI_HCFS_OPERATIONAL | hn->hn_HCD.OHCI.Remote_Wakeup_Connect );

	if ( ( PCI_READLONG( OHCI_CONTROL ) & OHCI_HCFS_MASK ) != OHCI_HCFS_OPERATIONAL )
	{
		goto bailout;
	}

	//
	// The controller is now OPERATIONAL. Set a some final
	// registers that should be set earlier, but that the
	// controller ignores when in the SUSPEND state.
	//

	ival = OHCI_GET_IVAL( hn->hn_HCD.OHCI.Frame_Interval );

	fm	= ( PCI_READLONG( OHCI_FM_REMAINING ) & OHCI_FIT ) ^ OHCI_FIT;
	fm |= OHCI_FSMPS( ival );
	fm |= ival;

	PCI_WRITELONG( OHCI_FM_INTERVAL, 		fm );
	PCI_WRITELONG( OHCI_PERIODIC_START, 	OHCI_PERIODIC( ival ));

	for ( portnr = 1 ; portnr <= hn->hn_HUB_NumPorts ; portnr++ )
	{
		if ( OHCI_Port_Set_Power( hn, portnr ) != USB2Err_NoError )
		{
			goto bailout;
		}
	}

	PCI_WRITELONG( OHCI_INTERRUPT_ENABLE, ( OHCI_WDH | OHCI_UE | OHCI_RHSC | OHCI_MIE ));

	// --

	retval = TRUE;

bailout:

	TASK_NAME_LEAVE();

	return( retval );
}

// --
