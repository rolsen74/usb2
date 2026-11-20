
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <exec/io.h>
#include <proto/exec.h>
#include "SDRTest.h"
#include "Playback.h"

// --

void
SDR_ClosePlayback( struct SDRPlayback *play )
{
	if ( play->Pending )
	{
		if ( ! CheckIO( (PTR) play->Request ) )
		{
			AbortIO( (PTR) play->Request );
		}

		WaitIO( (PTR) play->Request );
	}

	if ( play->Opened )
	{
		CloseDevice( (PTR) play->Request );
	}

	FreeVec( play->Playing );

	FreeVec( play->Staging );

	FreeSysObject( ASOT_IOREQUEST, play->Request );

	FreeSysObject( ASOT_PORT, play->Port );
}

// --
