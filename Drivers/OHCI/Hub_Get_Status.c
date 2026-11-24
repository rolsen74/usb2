
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

SEC_CODE U32
OHCI_Hub_Get_Status( struct USB2_HCDNode *hn )
{
	U32 status;

	status = PCI_READLONG( OHCI_RH_STATUS );

	return ( ( ( status & OHCI_OCI ) ? HUBSTATF_Over_Current : 0 ) |
		( ( status & OHCI_CCIC ) ? ( HUBCHGF_Over_Current_Chg << 16 ) : 0 ) );
}

// --
