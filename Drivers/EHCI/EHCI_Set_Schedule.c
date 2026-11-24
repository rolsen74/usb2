
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "EHCI.h"

// --

SEC_CODE static S32
WaitSchedule( struct USBBase *usbbase, struct USB2_HCDNode *hn, U32 mask, S32 enabled )
{
	U32 count;
	U32 status;

	for ( count = 0 ; count < 100 ; count++ )
	{
		status = PCI_READLONG( hn->hn_HCD.EHCI.CapLength + EHCI_USBSTS );

		if ( status & ( EHCI_STS_HCH | EHCI_STS_HSE ) )
		{
			return ( FALSE );
		}

		if ( ( ( status & mask ) != 0 ) == enabled )
		{
			return ( TRUE );
		}

		MISC_MICRODELAY( 50 );
	}

	return ( FALSE );
}

// --

SEC_CODE S32
EHCI_Set_Schedule( struct USB2_HCDNode *hn, U32 command_mask, U32 status_mask, S32 enabled )
{
	struct USBBase *usbbase;

	U32 address;
	U32 command;

	usbbase = hn->hn_USBBase;
	address = hn->hn_HCD.EHCI.CapLength + EHCI_USBCMD;

	command = PCI_READLONG( address );

	// Finish any previous command before changing the schedule enable bit.
	if ( ! WaitSchedule( usbbase, hn, status_mask, ( command & command_mask ) != 0 ) )
	{
		return ( FALSE );
	}

	if ( ( ( command & command_mask ) != 0 ) == enabled )
	{
		return ( TRUE );
	}

	command &= ~( command_mask | EHCI_CMD_IAAD );

	if ( enabled )
	{
		command |= command_mask;
	}

	__asm__ volatile ( "sync" ::: "memory" );

	if ( ( enabled ) && ( command_mask == EHCI_CMD_ASE ) )
	{
		// The controller's saved cursor can point to a queue head just unlinked.
		PCI_WRITELONG( hn->hn_HCD.EHCI.CapLength + EHCI_ASYNC, hn->hn_HCD.EHCI.ASync_Head->pqh_self_phy );
	}

	PCI_WRITELONG( address, command );

	return ( WaitSchedule( usbbase, hn, status_mask, enabled ) );
}

// --
