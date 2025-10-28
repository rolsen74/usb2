
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

SEC_CODE static S32 __Init_1_Add_HUB( struct USBBase *usbbase, struct USB2_HCDNode *hn, struct intern *in UNUSED )
{
struct RealFunctionNode *fn;
S32 retval;

	retval = FALSE;

	// -- Add Root HUB Entry

	fn = FUNCTION_ALLOC( hn, NULL, 0 );

	if ( ! fn )
	{
		USBERROR( "__myInit (HCD)           : Error: Allocating RootHub Function" );
		goto bailout;
	}

	fn->fkt_Speed = hn->hn_MaxSpeed;

	hn->hn_Function_Array[0] = fn;
	hn->hn_EndPoint_Array[0] = fn->fkt_ControlEndPoint;
	hn->hn_EndPoint_Array[16] = fn->fkt_ControlEndPoint;

	retval = TRUE;

bailout:

	return( retval );
}

// --
