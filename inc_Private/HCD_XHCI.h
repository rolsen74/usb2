
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef INC_PRIVATE_USB2_ALL_H
#error Include "usb2_all.h" first
#endif

#ifndef INC_PRIVATE_HCD_XHCI_H
#define INC_PRIVATE_HCD_XHCI_H

/***************************************************************************/

/***************************************************************************/

struct _XHCI
{
	int dummy;
};

/***************************************************************************/

extern const struct HCDFunctions XHCIFunctions;

/***************************************************************************/

#endif // INC_PRIVATE_HCD_XHCI_H
