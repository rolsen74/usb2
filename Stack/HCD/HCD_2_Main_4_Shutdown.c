
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static S32
__Shutdown( struct USBBase *usbbase, struct USB2_HCDNode *hn )
{
	struct RealFunctionNode *fn;

	fn = hn->hn_Function_Array[0];

	if ( fn )
	{
		if ( FUNCTION_FREE( fn ) != FSTAT_Okay )
		{
			return ( FALSE );
		}

		hn->hn_Function_Array[0] = NULL;
	}

	return ( TRUE );
}

// --
