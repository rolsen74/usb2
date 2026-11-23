
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "MSD.h"

// --

#define MSD_VERSION_TEXT_( value ) #value
#define MSD_VERSION_TEXT( value )  MSD_VERSION_TEXT_ ( value )

SEC_RODATA static const char MSD_Device_ID[]
	= "\0$VER: " MSD_Device_Name
	  " " MSD_VERSION_TEXT ( MSD_Device_Version ) "." MSD_VERSION_TEXT ( MSD_Device_Revision ) " (" MSD_Device_Date ")\r\n";

// --

SEC_CODE struct MSDBase *
MSD_Manager_Init ( struct MSDBase * MSDBase, PTR seglist, struct ExecIFace * IExec UNUSED )
{
	TASK_NAME_ENTER ( "MSD : MSD_Manager_Init" );
	//	USBERROR( "MSD : MSD_Manager_Init" );

	MSDBase->msdbase_Library.lib_Node.ln_Type = NT_DEVICE;
	MSDBase->msdbase_Library.lib_Node.ln_Pri  = 0;
	MSDBase->msdbase_Library.lib_Node.ln_Name = (STR)MSD_Device_Name,
	MSDBase->msdbase_Library.lib_Flags		  = LIBF_SUMUSED | LIBF_CHANGED;
	MSDBase->msdbase_Library.lib_Version	  = MSD_Device_Version;
	MSDBase->msdbase_Library.lib_Revision	  = MSD_Device_Revision;
	MSDBase->msdbase_Library.lib_IdString	  = (STR)( MSD_Device_ID + sizeof ( "\0$VER: " ) - 1 );
	MSDBase->msdbase_SegList				  = seglist;

	TASK_NAME_LEAVE ();
	return ( MSDBase );
}

// --
