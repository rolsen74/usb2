
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct FS_ObjNode *_fs_Node_Find(
	struct FS_Struct *fs UNUSED,
	struct FS_ObjNode *node,
	int32 *res2,
	STR name )
{
struct FS_ObjNode *retval;
struct FS_ObjNode *cur;
U32 len;

	retval = NULL;

	MYINFO( "PTP-FS : _fs_Node_Find" );

	if (( ! node ) || ( ! name ) || ( name[0] ))
	{
		MYERROR( "PTP-FS : NULL Pointer" );

		if ( res2 )
		{
			*res2 = ERROR_REQUIRED_ARG_MISSING;
		}

		goto bailout;
	}

	// Can I get away with only Dirs?
	if ( node->on_Type != FSO_TYPE_DIRECTORY )
	{
		MYERROR( "PTP-FS : Not a Directory Type" );

		if ( res2 )
		{
			*res2 = ERROR_OBJECT_WRONG_TYPE;
		}

		goto bailout;
	}
	
	len = strlen( name );

	cur = (PTR) GetHead( & node->on_Content_List );

	while( cur )
	{
		if (( len == cur->on_NameLen ) && ( ! memcmp( name, cur->on_Name, len )))
		{
			break;
		}
		else
		{
			cur = (PTR) GetSucc( (PTR) cur );
		}
	}

	if ( ! cur )
	{
		MYERROR( "PTP-FS : Object not found" );

		if ( res2 )
		{
			*res2 = ERROR_OBJECT_NOT_FOUND;
		}

		goto bailout;
	}
	
	retval = cur;

bailout:

	return( retval );
}

// --
