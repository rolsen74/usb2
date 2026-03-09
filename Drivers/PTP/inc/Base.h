
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_ALL_H
#error Include "inc/All.h" first
#endif

#ifndef INC_BASE_H
#define INC_BASE_H

// --

struct PTPBase
{
	struct Library		lib_Base;
	PTR					lib_SegList;
};

// --

struct PTPData
{
	int	dummy;
};

// --

struct TaskStartMsg
{
	U32					Result;
	struct Task *		Parent;
	U32					Mask;
};

// --

#endif
