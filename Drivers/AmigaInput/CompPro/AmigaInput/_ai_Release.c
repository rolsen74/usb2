
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "CompPro.h"

// --

U32 _AI_Release( struct AINSubIFace *Self )
{
	Self->Data.RefCount--;

	return( Self->Data.RefCount );
}

// --
