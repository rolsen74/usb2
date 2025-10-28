
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
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
