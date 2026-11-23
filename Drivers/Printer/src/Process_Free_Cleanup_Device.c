
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static void
__myProcess_CleanupDevice( struct PrinterUnit *unit )
{
	struct USB2_IORequest *usbio;
	struct IOStdReq *request;

	U32 index;

	USBERROR( "Printer : __myProcess_CleanupDevice" );

	for( index = 0; index < NR_of_TXs; index++ )
	{
		usbio = unit->unit_Bulk_Tx_Active[index];

		if ( usbio )
		{
			AbortIO( (PTR) usbio );
		}
	}

	for( index = 0; index < NR_of_TXs; index++ )
	{
		usbio = unit->unit_Bulk_Tx_Active[index];

		if ( ! usbio )
		{
			continue;
		}

		WaitIO( (PTR) usbio );

		if ( unit->unit_Buffer_Size )
		{
			FreeVec( usbio->io_UserData );
		}
		else
		{
			request = usbio->io_UserData;
			request->io_Actual = usbio->io_Actual;
			request->io_Error = ( request->io_Flags & IOPARF_ABORT ) ? IOERR_ABORTED : ParErr_NotOpen;

			ReplyMsg( (PTR) request );
		}
	}

	if ( ( unit->unit_Dot4_Ready ) && ( ! unit->unit_Detached ) )
	{
		Dot4_Shutdown( unit );
	}

	if ( unit->unit_Bulk_Rx_Resource )
	{
		USB2_EPRes_Release( unit->unit_Bulk_Rx_Resource );

		unit->unit_Bulk_Rx_Resource = NULL;
	}

	if ( unit->unit_Bulk_Tx_Resource )
	{
		while( RemHead( unit->unit_USB_IOReq_FreeList ) )
		{
		}

		USB2_EPRes_Release( unit->unit_Bulk_Tx_Resource );

		unit->unit_Bulk_Tx_Resource = NULL;
	}

	if ( unit->unit_USB_Register )
	{
		USB2_Reg_Unregister( unit->unit_USB_Register );

		unit->unit_USB_Register = NULL;
	}
}

// --
