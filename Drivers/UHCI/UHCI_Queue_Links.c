
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "UHCI.h"

// --

SEC_CODE void
UHCI_Link_QH( struct UHCI_QH *anchor, struct UHCI_QH *qh )
{
	struct UHCI_QH *tail;

	tail = anchor;

	while( tail->pqh_virt_next )
	{
		tail = tail->pqh_virt_next;
	}

	qh->qh_hlink = tail->qh_hlink;
	qh->pqh_anchor = anchor;
	tail->qh_hlink = LE_SWAP32( qh->pqh_self_phy | UHCI_PTR_QH );
	tail->pqh_virt_next = qh;
	qh->pqh_linked = TRUE;
}

// --

SEC_CODE S32
UHCI_Unlink_QH( struct UHCI_QH *qh )
{
	struct UHCI_QH *prev;

	prev = qh->pqh_anchor;

	while( ( prev ) && ( prev->pqh_virt_next != qh ) )
	{
		prev = prev->pqh_virt_next;
	}

	if ( ! prev )
	{
		return ( FALSE );
	}

	prev->qh_hlink = qh->qh_hlink;
	prev->pqh_virt_next = qh->pqh_virt_next;
	qh->pqh_linked = FALSE;

	return ( TRUE );
}

// --
