
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

SEC_CODE static void __Shutdown( struct USBBase *usbbase, struct USB2_HCDNode *hn, struct intern *in UNUSED )
{
struct RealFunctionNode *fn;

	// Yes all should have been shutdown now
	// lets free RootHub Function (not the driver)

	USBINFO( "__myMain (HCD)           : __Shutdown (4) ::" );
	TASK_NAME_ENTER( "HCD Main : __Shutdown" );

	fn = hn->hn_Function_Array[0];

	if ( FUNCTION_FREE(fn) == FSTAT_Okay )
	{
		hn->hn_Function_Array[0] = NULL;
	}
	else
	{
		USBERROR( "__myMain (HCD)           : Index #%ld :: --> Unable to free RootHub Function", hn->hn_HCDIndex );
	}

	TASK_NAME_LEAVE();
}

// --
