
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

void Driver_Main( struct USB_Driver *in )
{
U32 wait;
U32 mask;

	#ifdef DEBUG
	DebugPrintF( "Main Loop\n" );
	#endif

	in->Running = TRUE;

	// Stack
	wait = in->USB_Function_Bit;

	while( in->Running )
	{
		mask = Wait( wait );

		if ( mask & in->USB_Function_Bit )
		{
			Driver_Handle_Stack( in );
		}
	}
}

// --
