
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void _fs_Recursive_Change_Update( struct FS_Struct *fs UNUSED, struct FS_ObjNode *node )
{
struct DateStamp DS;		/* Get a copy of the current datestamp */
int32 res2;

	MYINFO( "PTP-FS : _fs_Recursive_Change_Update" );

	if ( node )
	{
		/* DataStamp() affects pr_Result2 */
		res2 = IoErr();
		DateStamp( & DS );
		SetIoErr( res2 );

		/*
		** We update all the way to the root dir.
		*/

		while( node )
		{
			node->on_Protection &= ~EXDF_ARCHIVE;		/* clear -A- bit */

			node->on_Date.ds_Tick	= DS.ds_Tick;
			node->on_Date.ds_Minute	= DS.ds_Minute;	/* update date */
			node->on_Date.ds_Days	= DS.ds_Days;

			node = node->on_Parent_dir;
		}
	}

}

// --
