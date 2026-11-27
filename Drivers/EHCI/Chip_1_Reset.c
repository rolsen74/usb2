
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
#include "EHCI.h"

// --

#define EHCI_SOC_USBMODE	   ( 0x1a8 - 0x100 )
#define EHCI_USBMODE_MODE_MASK 3
#define EHCI_USBMODE_IDLE	   0
#define EHCI_USBMODE_HOST	   3

SEC_CODE static S32
SetHostMode ( struct USB2_HCDNode * hn )
{
	struct USBBase * usbbase;

	U32 mode;

	usbbase = hn->hn_USBBase;

	PCI_WRITELONG ( EHCI_SOC_USBMODE, EHCI_USBMODE_HOST );

	HCD_WAIT_MS ( hn, 2 );

	mode = PCI_READLONG ( EHCI_SOC_USBMODE );

	if ( ( mode & EHCI_USBMODE_MODE_MASK ) != EHCI_USBMODE_HOST )
	{
		usbbase->usb_IExec->DebugPrintF ( "USB : EHCI HCD %lu could not enter host mode, USBMODE $%08lx\n", hn->hn_HCDIndex,
										  mode );

		return ( FALSE );
	}

	return ( TRUE );
}

// --

SEC_CODE S32
EHCI_Chip_Reset ( struct USB2_HCDNode * hn )
{
	S32 retval;
	S32 uses_usbmode;
	U32 cnt;
	U32 val;
	U32 mode;

	struct USBBase * usbbase = hn->hn_USBBase;
	TASK_NAME_ENTER ( "EHCI : EHCI_Chip_Reset" );

	retval = FALSE;
	uses_usbmode
		= ( hn->hn_HCDType == HCDTYPE_TABOR ) || ( hn->hn_HCDType == HCDTYPE_P5020 ) || ( hn->hn_HCDType == HCDTYPE_P5040 );
	mode = 0;

	if ( uses_usbmode )
	{
		mode = PCI_READLONG ( EHCI_SOC_USBMODE );

		// A previous reset can leave the controller idle, outside host mode.
		if ( ( mode & EHCI_USBMODE_MODE_MASK ) == EHCI_USBMODE_IDLE )
		{
			if ( ! SetHostMode ( hn ) )
			{
				goto bailout;
			}
		}
	}

	USBDEBUG ( "EHCI_Chip_Reset          : 1 : Reset Stop" );

	// --
	// EHCI : Needs an explicit Stop/Halt before Reset
	// (because the spec says you must halt before HCRESET)

	if ( ! hn->HCD_Functions.Chip_Stop ( hn ) )
	{
		usbbase->usb_IExec->DebugPrintF ( "USB : EHCI HCD %lu could not halt for reset, USBSTS $%08lx\n", hn->hn_HCDIndex,
										  PCI_READLONG ( hn->hn_HCD.EHCI.CapLength + EHCI_USBSTS ) );

		if ( uses_usbmode )
		{
			usbbase->usb_IExec->DebugPrintF (
				"USB : EHCI HCD %lu initial USBMODE $%08lx, current USBMODE $%08lx, USBCMD $%08lx\n", hn->hn_HCDIndex, mode,
				PCI_READLONG ( EHCI_SOC_USBMODE ), PCI_READLONG ( hn->hn_HCD.EHCI.CapLength + EHCI_USBCMD ) );
		}

		goto bailout;
	}

	// --
	// Reset controller

	USBDEBUG ( "EHCI_Chip_Reset          : 2 : Reset Chip" );

	val = PCI_READLONG ( hn->hn_HCD.EHCI.CapLength + EHCI_USBCMD );

	val |= EHCI_CMD_HCRESET;

	PCI_WRITELONG ( hn->hn_HCD.EHCI.CapLength + EHCI_USBCMD, val );

	// --
	// Small settle delay

	HCD_WAIT_MS ( hn, 10 ); // Wait 10ms

// --
#define MAXMS 20 // max 5x20 = 100ms

	for ( cnt = 0; cnt < MAXMS; cnt++ )
	{
		if ( ! ( PCI_READLONG ( hn->hn_HCD.EHCI.CapLength + EHCI_USBCMD ) & EHCI_CMD_HCRESET ) )
		{
			break;
		}
		else
		{
			HCD_WAIT_MS ( hn, 5 ); // Wait 5ms
		}
	}

	if ( cnt == MAXMS )
	{
		usbbase->usb_IExec->DebugPrintF ( "USB : EHCI HCD %lu reset timed out, USBCMD $%08lx\n", hn->hn_HCDIndex,
										  PCI_READLONG ( hn->hn_HCD.EHCI.CapLength + EHCI_USBCMD ) );
		goto bailout;
	}

#undef MAXMS
	// --

	if ( uses_usbmode )
	{
		USBDEBUG ( "EHCI_Chip_Reset          : 3 : Change Host Mode" );

		HCD_WAIT_MS ( hn, 10 ); // Wait 10ms

		if ( ! SetHostMode ( hn ) )
		{
			goto bailout;
		}
	}

	// --

	retval = TRUE;

bailout:

	TASK_NAME_LEAVE ();

	return ( retval );
}

// --
