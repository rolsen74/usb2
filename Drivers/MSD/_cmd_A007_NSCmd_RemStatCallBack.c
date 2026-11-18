
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
MSD_Cmd_A007_NSCmd_RemStatCallBack( struct USBBase *usbbase, struct IOStdReq *ioreq )
{
	struct MSDDisk *msddisk;
	struct IOStdReq *registered;

	msddisk = (PTR) ioreq->io_Unit;

	registered = MSD_Disk_RemoveStatCallback( usbbase, msddisk, ioreq, NULL );

	if ( ( ! registered ) && ( ioreq->io_Data ) )
	{
		registered = MSD_Disk_RemoveStatCallback( usbbase, msddisk, NULL, ioreq->io_Data );
	}

	ioreq->io_Actual = 0;
	ioreq->io_Error = registered ? 0 : TDERR_NotSpecified;

	if ( ( registered ) && ( registered != ioreq ) )
	{
		registered->io_Actual = 0;
		registered->io_Error = 0;

		MSGPORT_REPLYMSG( registered );
	}
}

// --
