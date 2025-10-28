
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#if 0
static S32 __myInit( struct USBBase *usbbase, PTR userdata UNUSED, PTR in UNUSED )
{
//struct RealFunctionNode *fn;
//struct USB2_HCDNode *hn;
S32 retval;

	retval = FALSE;

	USBDEBUG( "__myInit (Init) : Enter : DN %p", dn );

	// --

	if ( DRIVER_VALID( dn ) != VSTAT_Okay )
	{
		USBDEBUG( "__myInit: Invalid DN (%p)", dn );
		goto bailout;
	}

	// --

//	hn = dn->dn_HCD;
//
//	if ( HCD_VALID( hn ) != VSTAT_Okay )
//	{
//		USBDEBUG( "__myInit: Invalid HCD (%p)", hn );
//		goto bailout;
//	}

	// --

//	if ( FUNCTION_VALID(fn) != VSTAT_Okay )
//	{
//		USBDEBUG( "__myInit: Invalid Function" );
//		goto bailout;
//	}



	// --

	retval = TRUE;

	USBDEBUG( "__myMain (Init)          : Done" );

bailout:

	// -- Exit

	USBDEBUG( "__myMain (Init)          :: Leave" );

	return( retval );
}
#endif

// --
