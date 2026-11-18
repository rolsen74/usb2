
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct ExamineData *
_fs_ExamineNode( struct FS_ObjNode *node, struct PRIVATE_ExamineDirContext *context )
{
	struct ExamineData *data;

	data = AllocDosObjectTags( DOS_EXAMINEDATA,
		ADO_ExamineData_NameSize, node->on_NameLen + 1,
		ADO_ExamineData_CommentSize, 1,
		ADO_ExamineData_LinkSize, 1,
		context ? ADO_ExamineDir_Context : TAG_IGNORE, context,
		TAG_END );

	if ( ! data )
	{
		return ( NULL );
	}

	data->Type = node->on_Type;
	data->FileSize = node->on_Type == FSO_TYPE_FILE ? (S64) node->on_File_Size : -1;
	data->Date = node->on_Date;
	data->ObjectID = (U32) node;
	data->RefCount = 0;
	data->Protection = node->on_Protection;

	memcpy( data->Name, node->on_Name, node->on_NameLen + 1 );

	return ( data );
}

// --
