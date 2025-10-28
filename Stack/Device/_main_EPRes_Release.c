
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

static void _main_EPRes_Release( struct USB2_IFace *Self, struct USB2_EPResource *epr )
{
struct USBBase *usbbase;

	usbbase = (PTR) Self->Data.LibBase;

	if ( ! epr )
	{
		USBERROR( "_main_EPRes_Release : NULL Pointer" );
		goto bailout;
	}

	#ifdef DEBUG

	struct RealEndPointResource *real;
	real = (PTR) ( (char *) epr - offsetof( struct RealEndPointResource, epr_Public ));

	if ( ENDPOINTRES_VALID( real ) != VSTAT_Okay )
	{
		USBERROR( "_main_EPRes_Release : Error invalid EndPoint Resource" );
		goto bailout;
	}

	#endif

	ENDPOINTRES_RELEASE( epr );

bailout:

	return;
}

// --
