
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
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
