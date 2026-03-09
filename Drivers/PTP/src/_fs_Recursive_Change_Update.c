
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void _fs_Recursive_Change_Update( struct PTP_FSStruct *fs, struct FS_ObjNode *node )
{
struct DateStamp DS;		/* Get a copy of the current datestamp */
int32 res2;

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
			node->Protection &= ~EXDF_ARCHIVE;		/* clear -A- bit */

			node->Date.ds_Tick   = DS.ds_Tick;
			node->Date.ds_Minute = DS.ds_Minute;	/* update date */
			node->Date.ds_Days   = DS.ds_Days;

			node = node->Parent_dir;
		}
	}

}

// --
