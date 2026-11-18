
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

// Takes ownership of base and returns the allocated directory name.
STR
_fs_Unique_Name( struct FS_ObjNode *parent, STR base )
{
	STR name;
	U32 suffix;

	if ( ( ! base ) || ( ! _fs_Node_Find( parent, base, strlen( base ) ) ) )
	{
		return ( base );
	}

	for( suffix = 2; ; suffix++ )
	{
		name = ASPrintf( "%s-%lu", base, suffix );

		if ( ( ! name ) || ( ! _fs_Node_Find( parent, name, strlen( name ) ) ) )
		{
			break;
		}

		FreeVec( name );
	}

	FreeVec( base );

	return ( name );
}

// --
