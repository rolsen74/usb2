
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Firmware.h"

// --

void
RTL8188EU_StopFirmware( struct USB2_EPResource *control )
{
	RTL8188EU_Update8( control, 0x0100, 0xc0, 0 );

	RTL8188EU_Write8( control, 0x001f, 0 );

	RTL8188EU_Update8( control, RTL8188EU_SYS_FUNC_EN + 1, RTL8188EU_CPU_ENABLE, 0 );

	RTL8188EU_Write8( control, RTL8188EU_FW_CTL, 0 );
}

// --
