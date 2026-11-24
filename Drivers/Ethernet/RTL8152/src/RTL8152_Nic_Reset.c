
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

#define RTL8152_RESET_POLLS 50U

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

	for( cnt=0 ; cnt<RTL8152_RESET_POLLS ; cnt++ )
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
			IDOS->Delay( 1 );
		}
	}

	if ( cnt == RTL8152_RESET_POLLS )
	{
		IExec->DebugPrintF( "RTL8152: NIC reset did not complete\n" );
		goto bailout;
	}

	retval = TRUE;

bailout:
	return(		retval );
}

// --
