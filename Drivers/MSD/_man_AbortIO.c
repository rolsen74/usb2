
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "MSD.h"

// --

SEC_CODE void
MSD_Manager_AbortIO( struct DeviceManagerInterface *Self, struct IOStdReq *ioreq )
{
	struct MSDBase *msdbase;
	struct USBBase *usbbase;
	struct MSDDisk *msddisk;
	struct IOStdReq *registered;

	if ( ( ! ioreq ) || ( ioreq->io_Command != NSCMD_TD_ADDSTATCALLBACK ) || ( ! ioreq->io_Unit ) )
	{
		return;
	}

	msdbase = (PTR) Self->Data.LibBase;
	usbbase = msdbase->msdbase_usbbase;
	msddisk = (PTR) ioreq->io_Unit;

	registered = MSD_Disk_RemoveStatCallback( usbbase, msddisk, ioreq, NULL );

	if ( registered )
	{
		registered->io_Error = IOERR_ABORTED;
		registered->io_Actual = 0;

		MSGPORT_REPLYMSG( registered );
	}
}

// --
