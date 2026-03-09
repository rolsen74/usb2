
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

/*
** Note:
**  Do not allocate MsgPorts / signals here.
**  We’re running in the Ramdisk/Ramlib/Exec startup context. If you need to
**  allocate any signals, do it in manager_open (the caller’s process context).
*/

PTR _man_Init( PTR Dummy UNUSED, PTR SegList, struct ExecBase *mySysBase )
{
struct PTPBase *libBase;

	SysBase = (PTR) mySysBase;
	IExec = (PTR) mySysBase->MainInterface;

	MYDEBUG( "PTP : _manager_Init" );

	// Create library, but still not public
	extern const PTR libInterfaces[];
	libBase = (PTR) CreateLibraryTags(
		CLT_DataSize, sizeof( struct PTPBase ),
		CLT_Interfaces,	libInterfaces,
		CLT_NoLegacyIFace, TRUE,
		TAG_END
	);

	if ( ! libBase )
	{
		MYERROR( "PTP : _manager_Init : Failed to create device" );
		goto bailout;
	}

	libBase->lib_Base.lib_Node.ln_Type	= NT_LIBRARY;
	libBase->lib_Base.lib_Node.ln_Pri	= LIBPRI;
	libBase->lib_Base.lib_Node.ln_Name	= LIBNAME;
	libBase->lib_Base.lib_Flags			= LIBF_SUMUSED|LIBF_CHANGED;
	libBase->lib_Base.lib_Version		= VERSION;
	libBase->lib_Base.lib_Revision		= REVISION;
	libBase->lib_Base.lib_IdString		= VSTRING;
	libBase->lib_SegList = SegList;

	/*
	** Because we add the library manually, we have full control here.
	** If needed, you could obtain your own main/docky interface and call your
	** own methods before publication.
	*/

	if ( Resources_Init( libBase ))
	{
		MYDEBUG( "PTP : Opening Resources...  Success" );

		/* Make it public */
		AddLibrary( (PTR) libBase );

		/* Optional: small startup message */
		DebugPrintF( "PTP : Rock'n Roll\n" );
	}
	else
	{
		MYERROR( "PTP : Opening Resources...  Failed" );

		Resources_Free( libBase );

		DeleteLibrary( (PTR) libBase );
		libBase = NULL;
	}

bailout:

	return( libBase );
}

// --
