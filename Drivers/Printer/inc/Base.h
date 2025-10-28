
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
