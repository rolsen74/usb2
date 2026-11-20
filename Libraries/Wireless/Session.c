
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
Clear_Secret( void *memory, U32 length )
{
	volatile U8 *data;

	data = memory;

	while( length-- )
	{
		*data++ = 0;
	}
}

// --

void
Destroy_KeySession( struct WirelessKeySession *session )
{
	Crypto_Close( & session->Crypto );

	Clear_Secret( session, sizeof( *session ) );

	FreeVec( session );
}

// --
