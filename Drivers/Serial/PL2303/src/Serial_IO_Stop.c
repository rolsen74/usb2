/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_IO_Stop( struct SerialContext *ctx UNUSED, struct USB2_IORequest *ioreq )
{
	if ( ! CheckIO( (PTR) ioreq ) )
	{
		AbortIO( (PTR) ioreq );
	}

	WaitIO( (PTR) ioreq );
}

// --
