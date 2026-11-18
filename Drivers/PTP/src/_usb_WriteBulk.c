
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

S32
_usb_WriteBulk( struct USB_Struct *us, U32 length, S32 final )
{
	struct USB2_IORequest *io;

	io = us->us_Res_BulkOut->IORequests[0];
	io->io_Command = CMD_WRITE;
	io->io_Data = us->us_Res_BulkOut->Buffers[0];
	io->io_Length = length;
	io->io_AddZeroPackets = final;
	io->io_TimeOut = 5000000;

	DoIO( (PTR) io );

	if ( io->io_Error == USB2Err_Device_Detached )
	{
		us->us_Detached = TRUE;
	}

	return ( ( ! io->io_Error ) && ( io->io_Actual == length ) );
}

// --
