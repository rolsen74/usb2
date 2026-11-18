/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE S32 Serial_Vendor_Read( struct SerialContext *ctx, struct SerialUnit *unit, U16 value, U8 *result )
{
	return ( Serial_Control( ctx, unit, REQTYPE_Read | REQTYPE_Vendor | REQTYPE_Device, 1, value, 0, result, 1 ) );
}

// --
