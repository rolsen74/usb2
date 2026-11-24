
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

SEC_CODE void
OHCI_Hub_Clr_OverCurrent_Chg( struct USB2_HCDNode *hn )
{
	PCI_WRITELONG( OHCI_RH_STATUS, OHCI_CCIC );
}

// --
