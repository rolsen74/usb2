
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_BASE_H
#define INC_BASE_H

// --

#include "All.h"

// --

#define MAX_PRT_UNITS			32

struct PrinterBase
{
	struct Device				dev_Base;
	PTR							dev_SegList;
	struct SignalSemaphore 		dev_PRT_Semaphore;
	PTR							dev_PRT_Units[MAX_PRT_UNITS];
};

// --

#endif
