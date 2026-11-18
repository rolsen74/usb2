
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

STR
_fs_Camera_Name( struct FS_Struct *fs, struct CameraInfoNode *camera )
{
	STR base;

	if ( ( camera->cin_Apple ) && ( ! strcmp( camera->cin_Name, "Apple iPhone" ) ) )
	{
		base = ASPrintf( "%s", "iPhone" );
	}
	else if ( camera->cin_Name[0] )
	{
		base = ASPrintf( "%s", camera->cin_Name );
	}
	else
	{
		base = ASPrintf( "Camera-%08lx%08lx", (U32) ( camera->cin_ID >> 32 ), (U32) camera->cin_ID );
	}

	return ( _fs_Unique_Name( fs->fs_RootNode, base ) );
}

// --
