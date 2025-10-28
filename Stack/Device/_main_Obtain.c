
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static U32 _main_Obtain( struct Interface *Self )
{
	Self->Data.RefCount--;

	return( Self->Data.RefCount );
}

// --
