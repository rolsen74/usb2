
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct FS_ObjNode *_fs_Node_Locate_Rel(
	struct FS_Struct *fs,
	struct FS_ObjNode **parent,
	struct FS_ObjLock *lock,
	int32 follow_links,
	int32 *res2,
	STR name )
{
struct FS_ObjNode *retval;
struct FS_ObjNode *dptr;
U32 len;

	MYINFO( "PTP-FS : _fs_Node_Locate_Rel" );

	retval = NULL;

	/*
	**  Must handle the relative lock first. !!!
	**  If lock is non-zero, use the lock node.
	**  If lock is NULL, use the root dir node.
	*/

	if ( lock )
	{
		/* extract object node from lock */
		dptr = lock->Node;
	}
	else
	{
		/* handle zero relative root locks here */
		dptr = fs->fs_RootNode;
	}

	/*
	**  This can't really happen, but check it anyway.
	*/
	
	if ( ! dptr )
	{
		MYERROR( "PTP-FS : NULL Pointer" );

		if ( res2 )
		{
			*res2 = ERROR_REQUIRED_ARG_MISSING;
		}
		goto bailout;
	}

	/*
	**  This can't really happen either, but check it anyway.
	*/
	
	if ( ! name )
	{
		MYERROR( "PTP-FS : NULL Pointer" );

		if ( res2 )
		{
			*res2 = ERROR_REQUIRED_ARG_MISSING;
		}
		goto bailout;
	}

	/*
	**  If the name is an empty string ("").
	**  We return the current relative node, REGARDLESS of type
	**  because it can also be a file node.
	*/

	if ( name[0] == 0 )
	{
		retval = dptr;
		goto bailout;
	}

	/*
	**  Now, if we are going to search a directory for our named object,
	**  we must make sure we are using a directory as the relative reference.
	*/

	if ( dptr->on_Type != FSO_TYPE_DIRECTORY )
	{
		MYERROR( "PTP-FS : Not a Directory Type" );

		if ( res2 )
		{
			*res2 = ERROR_OBJECT_WRONG_TYPE;
		}
		goto bailout;
	}

	retval = (PTR) GetHead( & dptr->on_Content_List );
	len = strlen( name );

	while( retval )
	{
		if (( len == retval->on_NameLen ) && ( ! memcmp( name, retval->on_Name, len )))
		{
			break;
		}
		else
		{
			retval = (PTR) GetSucc( (PTR) retval );
		}
	}

	if ( ! retval )
	{
		MYERROR( "PTP-FS : Object not found" );

		if ( res2 )
		{
			*res2 = ERROR_OBJECT_NOT_FOUND;
		}
		goto bailout;
	}

	/*
	** Handle hard and soft links resolution.
	*/

	if ( follow_links )
	{
		if ( retval->on_Type & FSOF_LINK )
		{
			switch( retval->on_Type & FSO_TYPE_MASK )
			{
				case FSO_TYPE_SOFTLINK:
				{
					/* This error code is also required */

					if ( res2 )
					{
						*res2 = ERROR_IS_SOFT_LINK;
					}
					
					/* Must return NULL it's a softlink. */
					retval = NULL;
					break;
				}

				case FSO_TYPE_FILE:
				case FSO_TYPE_DIRECTORY:
				{
					retval = retval->on_HardLink_Target;
					break;
				}

				default:
				{
					break;
				}
			}
		}
	}

bailout:

	if ( parent )
	{
		*parent = dptr;
	}

	return(	retval );
}

// --
