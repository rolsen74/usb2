
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

#define MAX_ETH_UNITS			32

struct EthernetBase
{
	struct Device				eth_Base;
	PTR							eth_SegList;
	struct SignalSemaphore 		eth_ETH_Semaphore;
	PTR							eth_ETH_Units[MAX_ETH_UNITS];
};

// --

#endif
