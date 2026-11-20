
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static struct USB2_Interface *
FindInterface( U32 unitnr )
{
	struct USB2_Interface *ifc;

	U32 index;

	for( index = 0;; index++ )
	{
		ifc = USB2_Ifc_FindTags( USB2Tag_Find_VendorID, RT2870_VENDOR,
			USB2Tag_Find_DeviceID, RT2870_PRODUCT, USB2Tag_Find_SeeClaimed, TRUE,
			USB2Tag_Find_Index, index, TAG_END );

		if ( ! ifc )
		{
			return ( NULL );
		}

		if ( ifc->IFC_Num == 0 )
		{
			if ( ! unitnr )
			{
				return ( ifc );
			}

			unitnr--;
		}
	}
}

// --

S32
Process_Init( struct EthernetUnit *unit )
{
	struct USB2_Interface *ifc;
	struct USB2_Register *reg;

	ifc = FindInterface( unit->unit_UnitNr );

	if ( ! ifc )
	{
		USBERROR( "RT2870: no 07D1:3C09 adapter for unit %lu", unit->unit_UnitNr );

		return ( FALSE );
	}

	reg = USB2_Reg_RegisterTags( USB2Tag_Reg_Interface, ifc,
		USB2Tag_Reg_Title, "RT2870", TAG_END );

	if ( ! reg )
	{
		USBERROR( "RT2870: unit %lu USB registration failed", unit->unit_UnitNr );

		return ( FALSE );
	}

	unit->unit_USB_Register = reg;

	return ( RT2870_Probe( unit ) );
}

// --
