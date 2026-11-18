
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Controller.h"

// --

int
RTL8152_Nic_Reset( struct devUnit *unit )
{
U32 cnt;
uint8 val;
int retval;

	retval = FALSE;

	if ( myWrite1( unit, URE_PLA_CR, URE_MCU_TYPE_PLA, URE_CR_RST ))
	{
		SHOWMSG( "Write Error" );
		goto bailout;
	}

	for( cnt=0 ; cnt<URE_TIMEOUT ; cnt++ )
	{
		if ( myRead1( unit, URE_PLA_CR, URE_MCU_TYPE_PLA, & val ))
		{
			SHOWMSG( "Read Error" );
			goto bailout;
		}

		if (( val & URE_CR_RST ) == 0 )
		{
			break;
		}
		else
		{
			IDOS->Delay( 5 );
		}
	}

	if ( cnt == URE_TIMEOUT )
	{
		SHOWMSG( "Reset Timeout" );
		goto bailout;
	}

	retval = TRUE;

bailout:
	return(		retval );
}

// --
