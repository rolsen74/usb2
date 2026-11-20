
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
Process_Free( struct EthernetUnit *unit )
{
	struct Message *msg;
	struct Node *node;

	S32 stopRadio;

	ObtainSemaphore( & unit->unit_CommandLock );

	stopRadio = ( ! unit->unit_Detached ) && ( unit->unit_RadioInitialized );
	unit->unit_Detached = TRUE;

	AbortPendingCommands( unit, NULL, NULL );

	while(( node = RemHead( & unit->unit_Multicast ) ))
	{
		FreeVec( node );
	}

	ReleaseSemaphore( & unit->unit_CommandLock );

	SiS163U_StopRX( unit );

	if ( stopRadio )
	{
		SiS163U_StopRadio( unit );
	}

	if ( unit->unit_Bulk_Tx_Resource )
	{
		USB2_EPRes_Release( unit->unit_Bulk_Tx_Resource );
	}

	if ( unit->unit_Bulk_Rx_Resource )
	{
		USB2_EPRes_Release( unit->unit_Bulk_Rx_Resource );
	}

	if ( unit->unit_USB_Register )
	{
		while(( msg = GetMsg( unit->unit_USB_Register->Stack_MsgPort ) ))
		{
			ReplyMsg( msg );
		}

		USB2_Reg_Unregister( unit->unit_USB_Register );
	}
}

// --
