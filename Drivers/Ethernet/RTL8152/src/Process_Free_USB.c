
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
unit_Process_FreeUSB( struct devUnit *unit )
{
	struct USB2IFace *IUSB2;
	struct IONode *node;

	IUSB2 = unit->unit_IUSB2;

	while(( node = (PTR) IExec->RemHead( & unit->unit_USBBulkInActiveList ) ))
	{
		FreeIONode( unit, node );
	}

	while(( node = (PTR) IExec->RemHead( & unit->unit_USBBulkOutActiveList ) ))
	{
		FreeIONode( unit, node );
	}

	while(( node = (PTR) IExec->RemHead( & unit->unit_USBBulkInFreeList ) ))
	{
		FreeIONode( unit, node );
	}

	while(( node = (PTR) IExec->RemHead( & unit->unit_USBBulkOutFreeList ) ))
	{
		FreeIONode( unit, node );
	}

	if ( unit->unit_USBBulkIn )
	{
		IUSB2->USB2_EPRes_Release( unit->unit_USBBulkIn );
	}

	if ( unit->unit_USBBulkOut )
	{
		IUSB2->USB2_EPRes_Release( unit->unit_USBBulkOut );
	}

	if ( unit->unit_USBRegister )
	{
		IUSB2->USB2_Reg_Unregister( unit->unit_USBRegister );
	}
}

// --
