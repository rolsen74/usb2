
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
OHCI_GetToggle( struct OHCI_ED *ed )
{
	struct OHCI_TD *td;

	U32 head;
	U32 toggle;

	head = LE_SWAP32( ed->ed_headp );

	for ( td = ed->ped_first_td ; ( td ) && ( td->ptd_virt_next ) ; td = td->ptd_virt_next )
	{
		if ( td->ptd_self_phy == ( head & OHCI_HEADMASK ) )
		{
			toggle = LE_SWAP32( td->td_flags ) & OHCI_TD_TOGGLE_MASK;

			if ( ( toggle == OHCI_TD_TOGGLE_0 ) || ( toggle == OHCI_TD_TOGGLE_1 ) )
			{
				return ( toggle == OHCI_TD_TOGGLE_1 );
			}

			break;
		}
	}

	return ( ( head & OHCI_TOGGLECARRY ) != 0 );
}

// --
