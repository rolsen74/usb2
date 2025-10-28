
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
/* -- Main Interface -- */

static U32 _main_EPRes_Destall( struct USB2_IFace *Self, struct USB2_EPResource *epr )
{
struct USBBase *usbbase;
U32 retval;

	retval = FALSE;
	usbbase = (PTR) Self->Data.LibBase;

	if ( ! epr )
	{
		USBERROR( "_main_EPRes_Destall : NULL Pointer" );
		goto bailout;
	}

	#ifdef DEBUG

	struct RealEndPointResource *real;
	real = (PTR) ( (char *) epr - offsetof( struct RealEndPointResource, epr_Public ));

	if ( ENDPOINTRES_VALID( real ) != VSTAT_Okay )
	{
		USBERROR( "_main_EPRes_Destall : Error invalid EndPoint Resource" );
		goto bailout;
	}

	#endif

	retval = ENDPOINTRES_DESTALL( epr );

bailout:

	return( retval );
}
