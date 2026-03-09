
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

S32 Resources_Init( struct PTPBase *libBase UNUSED )
{
struct TaskStartMsg msg;
struct Task *task;
S32 retval;
U32 mask;
U32 ver;

	MYDEBUG( "PTP : Resources_Init" );

	retval = FALSE;

	// --
	// Open Resources first
	//
	if ( ! myOpenResources( INT_MAX ))
	{
		MYERROR( "PTP : myOpenResources() failed" );
		goto bailout;
	}

	// --
	// File Handler need DOS 53.95+

	ver = (( DOSBase->lib_Version  << 16 ) | ( DOSBase->lib_Revision << 0 ));

	if ( ver < (( 53 << 16 ) | (( 95 << 0 ))))
	{
		MYERROR( "PTP : Need DOS v53.95+" );
		goto bailout;
	}

	// --

	msg.Parent	= FindTask( NULL );
	msg.Result	= FALSE;
	msg.Mask	= SIGBREAKF_CTRL_E;

	task = (PTR) CreateNewProcTags( 
		NP_Name,		"PTP Master",
		NP_Child,		FALSE,
		NP_Priority,	5,
		NP_StackSize,	1024 * 32,
		NP_Entry,		Master_Entry,
		NP_UserData,	& msg,
		TAG_END
	);

	while( task )
	{
		mask = Wait( msg.Mask );

		if (( mask & msg.Mask ) == msg.Mask )
		{
			retval = msg.Result;
			break;
		}
	}

	// --

bailout:

	return( retval );
}

// --