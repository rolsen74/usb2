
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Version.h"

// --

struct devBase *
Device_Init( struct devBase *devBase, APTR seglist, struct ExecIFace *myIExec )
{
struct devBase *retval;

	retval = NULL;

	IExec = myIExec;

	// Okey we passed, lets start our init routines

	IExec->Obtain();

	devBase->dev_Base.dd_Library.lib_Node.ln_Type = NT_DEVICE;
	devBase->dev_Base.dd_Library.lib_Node.ln_Name = (APTR) DEVNAME;
	devBase->dev_Base.dd_Library.lib_Flags		= LIBF_SUMUSED|LIBF_CHANGED;
	devBase->dev_Base.dd_Library.lib_Version	= VERSION;
	devBase->dev_Base.dd_Library.lib_Revision	= REVISION;
	devBase->dev_Base.dd_Library.lib_IdString	= (APTR) VSTRING;
	devBase->dev_SegList	= seglist;

	SHOWMSG( "Opening Resources..." );

	if ( OpenResources( devBase ))
	{
		SHOWMSG( "Opening Resources...  Successed" );

		retval = devBase;
	}
	else
	{
		SHOWMSG( "Opening Resources...  Failed" );

		CloseResources( devBase );

		IExec->Release();
	}
	return(		retval );
}

// --
