
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

S32 Driver_AI_Add( struct USB_Driver *in )
{
S32 retval;

	retval = FALSE;

	// --
	DebugPrintF( "Driver_AI_Add 1\n" );

	in->AI_IFace = (PTR) MakeInterface( (PTR) in->libBase, AI_Tags );

	if ( ! in->AI_IFace )
	{
		#ifdef DEBUG
		DebugPrintF( "Error creating AmigaInput interface\n" );
		#endif
		goto bailout;
	}

	// --
	DebugPrintF( "Driver_AI_Add 2\n" );

	in->AI_Driver = AIN_AddDriver( in->AI_IFace, "Competition Pro" );

	if ( ! in->AI_Driver )
	{
		#ifdef DEBUG
		DebugPrintF( "Error adding AmigaInput driver\n" );
		#endif
		goto bailout;
	}

	// --

	DebugPrintF( "Driver_AI_Add 99\n" );

	retval = TRUE;

bailout:

	return( retval );
}

// --
