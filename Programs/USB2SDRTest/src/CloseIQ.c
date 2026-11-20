
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "SDRTest.h"

// --

S32
SDR_CloseIQ( struct USB2_EPResource *control, struct USB2_EPResource *bulk )
{
	S32 stopped;

	stopped = TRUE;

	if ( SDR_HoldFIFO( control, TRUE ) )
	{
		stopped = FALSE;
	}

	if ( ( SDR_SetI2CGate( control, TRUE ) ) || ( ! SDR_R820T_Standby( control ) ) )
	{
		stopped = FALSE;
	}

	if ( SDR_SetI2CGate( control, FALSE ) )
	{
		stopped = FALSE;
	}

	if ( SDR_PowerOff( control ) )
	{
		stopped = FALSE;
	}

	USB2_EPRes_Release( bulk );

	return ( stopped );
}

// --
