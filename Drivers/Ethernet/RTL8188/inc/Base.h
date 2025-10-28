
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
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
