
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

PTR _manager_Init( struct libBase *libBase, PTR seglist, struct ExecIFace *myIExec )
{

	IExec = myIExec;

	#ifdef DEBUG
	DebugPrintF( "Printer : _manager_Init\n" );
	#endif

	libBase->lib_Base.lib_Node.ln_Type	= NT_LIBRARY;
	libBase->lib_Base.lib_Node.ln_Pri	= 0;
	libBase->lib_Base.lib_Node.ln_Name	= LIBNAME;
	libBase->lib_Base.lib_Flags			= LIBF_SUMUSED|LIBF_CHANGED;
	libBase->lib_Base.lib_Version		= VERSION;
	libBase->lib_Base.lib_Revision		= REVISION;
	libBase->lib_Base.lib_IdString		= VSTRING;
	libBase->lib_SegList	= seglist;

	if ( Resources_Setup( libBase ))
	{
		#ifdef DEBUG
		DebugPrintF( "Printer : Opening Resources...  Success\n" );
		#endif

		return( (PTR) libBase );
	}
	else
	{
		#ifdef DEBUG
		DebugPrintF( "Printer : Opening Resources...  Failed\n" );
		#endif

		Resources_Cleanup( libBase );

		return( NULL );
	}
}

// --
