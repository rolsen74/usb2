
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct FS_ObjNode *_fs_Node_CreateDir( struct PTP_FSStruct *fs, S32 *res2, STR oname )
{
struct FS_ObjNode *node;
S32 errcode;
S32 retval;

	retval = FALSE;

	node = AllocVecTags( sizeof( struct FS_ObjNode ),
		AVT_Clear, 0,
		TAG_END
	);

	if ( ! node )
	{
		errcode = 0;
		MYERROR( "PTP-FS : Error allocating memory" );
		goto bailout;
	}

	NewList( & node->Content_List );

	// Do we really need to copy the string?
	node->Name = ASPrintf( "%s", oname );

	if ( ! node->Name )
	{
		errcode = 0;
		MYERROR( "PTP-FS : Error allocating memory" );
		goto bailout;
	}

	node->NameLen = strlen( oname );

	// node->Type = FSO_TYPE_DIRECTORY;

	// /* set creation date */
	// DateStamp( & node->Date );

	// Set no error
	errcode = 0;

	retval = TRUE;

bailout:

	if (( ! retval ) && ( node ))
	{
		_fs_Node_Delete( fs, node );
		node = NULL;
	}

	if ( res2 )
	{
		*res2 = errcode;
	}

	return( node );
}
