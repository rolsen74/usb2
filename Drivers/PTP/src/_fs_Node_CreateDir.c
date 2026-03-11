
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct FS_ObjNode *_fs_Node_CreateDir( struct FS_Struct *fs, int32 *res2, STR name )
{
struct FS_ObjNode *node;
S32 retval;
S32 err;

	MYINFO( "PTP-FS : _fs_Node_CreateDir" );

	node = NULL;

	retval = FALSE;

	if (( ! name ) || ( ! name[0] ))
	{
		MYERROR( "PTP-FS : Error name missing" );
		err = ERROR_REQUIRED_ARG_MISSING;
		goto bailout;
	}

	node = AllocVecTags( sizeof( struct FS_ObjNode ),
		AVT_Clear, 0,
		TAG_END
	);

	if ( ! node )
	{
		MYERROR( "PTP-FS : Error allocating memory" );
		err = ERROR_NO_FREE_STORE;
		goto bailout;
	}

	NewList( & node->on_Content_List );

	node->on_Name = ASPrintf( "%s", name );

	if ( ! node->on_Name )
	{
		MYERROR( "PTP-FS : Error allocating memory" );
		err = ERROR_NO_FREE_STORE;
		goto bailout;
	}

	node->on_NameLen = strlen( name );

	node->on_Type = FSO_TYPE_DIRECTORY;

	/* set creation date */
	DateStamp( & node->on_Date );

	// Set no error
	err = 0;

	retval = TRUE;

bailout:

	if (( ! retval ) && ( node ))
	{
		_fs_Node_Delete( fs, node );
		node = NULL;
	}

	if ( res2 )
	{
		*res2 = err;
	}

	return( node );
}

// --
