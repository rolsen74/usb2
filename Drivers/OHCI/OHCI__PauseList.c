
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "OHCI.h"

// --

SEC_CODE S32
OHCI_WaitFrames( struct USB2_HCDNode *hn )
{
	struct USBBase *usbbase;

	U16 frame;
	U32 retry;

	usbbase = hn->hn_USBBase;

	frame = PCI_READLONG( OHCI_FM_NUMBER );

	for ( retry = 0 ; retry < 25 ; retry++ )
	{
		HCD_WAIT_MS( hn, 1 );

		if ( (U16) ( PCI_READLONG( OHCI_FM_NUMBER ) - frame ) >= 2 )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
}

// --

SEC_CODE S32
OHCI_PauseList( struct USB2_HCDNode *hn, U32 control, U32 enable )
{
	PCI_WRITELONG( OHCI_CONTROL, control & ~enable );

	if ( ( PCI_READLONG( OHCI_CONTROL ) & ( OHCI_HCFS_MASK | enable ) ) != OHCI_HCFS_OPERATIONAL )
	{
		return ( FALSE );
	}

	return ( OHCI_WaitFrames( hn ) );
}

// --
