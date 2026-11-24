
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

SEC_CODE S32
EHCI_Has_Root_TT( const struct USB2_HCDNode *hn )
{
	return ( ( hn->hn_HCDType == HCDTYPE_P5020 ) || ( hn->hn_HCDType == HCDTYPE_P5040 ) );
}

// --

SEC_CODE S32
EHCI_Get_TT ( struct RealFunctionNode *fn, U32 *hub, U32 *port )
{
	struct RealFunctionNode *parent;

	parent = fn->fkt_Parent;

	while ( ( parent ) && ( parent->fkt_Tier ) )
	{
		if ( parent->fkt_Speed == USBSPEED_High )
		{
			*hub = parent->fkt_Address;
			*port = fn->fkt_PortNr;

			return ( ( *hub != 0 ) && ( *port != 0 ) );
		}

		fn = parent;
		parent = parent->fkt_Parent;
	}

	if ( ( parent ) && ( fn->fkt_PortNr ) && ( EHCI_Has_Root_TT( fn->fkt_HCD ) ) )
	{
		*hub = 0;
		*port = fn->fkt_PortNr;

		return ( TRUE );
	}

	return ( FALSE );
}

// --
