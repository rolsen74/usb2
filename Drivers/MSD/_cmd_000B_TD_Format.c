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

SEC_CODE S32
MSD_Cmd_000B_TD_Format( struct USBBase *usbbase, struct MSDDevice *msddev, struct IOStdReq *ioreq )
{
	ioreq->io_Actual = 0;

	return ( MSD_Disk_TransferBlocks( usbbase, msddev, ioreq, ioreq->io_Offset, TRUE ) );
}

// --
