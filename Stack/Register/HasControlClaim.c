
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

// The caller holds the stack lock. Skip the claim being considered for release.
SEC_CODE S32
Register_HasControlClaim( const struct RealRegister *reg, const struct USB2_InterfaceHeader *skip )
{
	struct RealFunctionNode *fn;
	struct USB2_ConfigNode *cn;
	struct USB2_InterfaceGroup *ig;
	struct USB2_InterfaceHeader *ih;

	fn = reg->reg_Function;
	cn = fn->fkt_Config_Active;

	if ( fn->fkt_Owner == reg )
	{
		return ( TRUE );
	}

	if ( ! cn )
	{
		return ( FALSE );
	}

	for( ig = cn->cfg_InterfaceGroups.uh_Head; ig; ig = Node_Next( ig ) )
	{
		for( ih = ig->ig_Group.uh_Head; ih; ih = Node_Next( ih ) )
		{
			if ( ( ih != skip ) && ( ih->ih_Owner == reg ) )
			{
				return ( TRUE );
			}
		}
	}

	return ( FALSE );
}

// --
