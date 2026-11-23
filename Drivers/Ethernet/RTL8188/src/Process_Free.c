
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"

// --

void
Process_Free( struct EthernetUnit *unit )
{
	S32 stop_firmware;

	ObtainSemaphore( & unit->unit_CommandLock );

	stop_firmware = ( ! unit->unit_Detached ) && ( unit->unit_FirmwareRunning );
	unit->unit_Detached = TRUE;
	unit->unit_StationAddressValid = FALSE;
	unit->unit_RadioReady = FALSE;
	unit->unit_TxReady = FALSE;
	unit->unit_WiFiStartPending = FALSE;

	RTL8188EU_DropLink( unit );

	Sana2_AbortPending( & unit->unit_Pending, NULL, NULL, IOERR_ABORTED, 0 );

	ReleaseSemaphore( & unit->unit_CommandLock );

	RTL8188EU_StopRX( unit );

	if ( stop_firmware )
	{
		RTL8188EU_StopFirmware( unit->unit_USB_Register->Res_Control );
	}

	unit->unit_FirmwareRunning = FALSE;

	if ( unit->unit_Bulk_Rx_Resource )
	{
		USB2_EPRes_Release( unit->unit_Bulk_Rx_Resource );

		unit->unit_Bulk_Rx_Resource = NULL;
	}

	if ( unit->unit_Bulk_Tx_Resource )
	{
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
