
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_BUFFER_H
#define INC_BUFFER_H

// --

#include "All.h"

// --

struct PrtBuffer
{
	struct Node		pb_Node;
	U32				pb_Pos;
	U32				pb_Free;
	U8				pb_Data[1];
};

// --

#endif
