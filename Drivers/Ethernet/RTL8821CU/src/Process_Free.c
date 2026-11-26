
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
Process_Free( struct EthernetUnit *unit, struct USB2IFace *usb2 )
{
	U32 index;
	S32 attached;

	ObtainSemaphore( & unit->unit_CommandLock );

	attached = ! unit->unit_Detached;
	unit->unit_Detached = TRUE;
	unit->unit_Online = FALSE;
	unit->unit_Handshake = FALSE;

	memset( unit->unit_Keys, 0, sizeof( unit->unit_Keys ) );

	Sana2_AbortPending( & unit->unit_Pending, NULL, NULL, IOERR_ABORTED, 0 );

	ReleaseSemaphore( & unit->unit_CommandLock );

	RTL8821CU_StopRX( unit );

	if ( unit->unit_FirmwareRunning )
	{
		if ( attached )
		{
			RTL8821CU_PowerOff( unit->unit_USB_Register->Res_Control );
		}

		unit->unit_FirmwareRunning = FALSE;
	}

	for( index = 0; index < RTL8821CU_TX_PIPES; index++ )
	{
		if ( unit->unit_Bulk_Out[index] )
		{
			usb2->USB2_EPRes_Release( unit->unit_Bulk_Out[index] );

			unit->unit_Bulk_Out[index] = NULL;
		}
	}

	if ( unit->unit_Bulk_In )
	{
		usb2->USB2_EPRes_Release( unit->unit_Bulk_In );

		unit->unit_Bulk_In = NULL;
	}

	if ( unit->unit_USB_Register )
	{
		usb2->USB2_Reg_Unregister( unit->unit_USB_Register );

		unit->unit_USB_Register = NULL;
	}
}

// --
