/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef INTERFACES_USB2UNITS_H
#define INTERFACES_USB2UNITS_H

#include <stdint.h>

// IOStdReq on the device's query unit. io_Actual returns the required size.
#define USB2_CMD_ENUM_UNITS 0x8010
#define USB2_UNIT_LIST_VERSION 1 // io_Offset
#define USB2_UNIT_BUSY 1
#define USB2_FIRST_UNIT 65537UL

struct USB2UnitInfo
{
	uint32_t Number;
	uint32_t Flags;
};

#endif
