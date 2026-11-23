/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE void Serial_Cmd_Clear( struct SerialContext *ctx, struct SerialUnit *unit, struct IOExtSer *ioreq )
{
	unit->RxHead = 0;
	unit->RxTail = 0;
	unit->Overrun = FALSE;

	Serial_Read_Start( ctx, unit );

	Serial_Reply( ctx, ioreq );
}

// --
