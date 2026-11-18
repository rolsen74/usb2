
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

#if defined( DO_ERROR ) || defined( DO_INFO ) || defined( DO_DEBUG )

static struct Task *__TASK = NULL;

void VARARGS68K
_MYDEBUG( U32 lvl UNUSED, STR fmt, ... )
{
struct Task *self;

va_list ap;
STR buf;

	// lvl : 0 = Debug, 1 = Info, 2 = Error

	buf = NULL;

	if ( IUtility )
	{
		va_start( ap, fmt );

		buf = VASPrintf( fmt, va_getlinearva( ap, PTR ));

		va_end( ap );
	}

	// ---

	Disable();

	self = FindTask(NULL);

	DebugPrintF( "%s%p: %s\n",
		( self != __TASK ) ? "--\n" : "", 
		self,
		( buf ) ? buf : (STR) fmt
	);

	__TASK = self;

	Enable();

	// ---

	if ( buf )
	{
		FreeVec( buf );
	}

	//-
}

#endif

// --
