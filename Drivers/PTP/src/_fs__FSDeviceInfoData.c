
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

int32
_fs__FSDeviceInfoData( struct FSVP *vp, int32 *res2, struct InfoData *info )
{
	struct FS_Struct *fs;
	struct FS_ObjNode *root;

	fs = vp->FSV.FSPrivate;

	memset( info, 0, sizeof( *info ) );

	info->id_DiskState = ID_DISKSTATE_WRITE_PROTECTED;
	info->id_BytesPerBlock = 512;
	info->id_DiskType = fs->fs_VolumeNode->vn_DiskType;
	info->id_VolumeNode = MKBADDR( fs->fs_VolumeNode );

	ObtainSemaphore( & fs->fs_Semaphore );

	for( root = (PTR) GetHead( & fs->fs_RootNode->on_Content_List ); root;
		root = (PTR) GetSucc( & root->on_Node ) )
	{
		if ( ( root->on_Camera ) && ( root->on_Camera->cin_CanUpload ) )
		{
			info->id_DiskState = ID_DISKSTATE_VALIDATED;
			break;
		}
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	if ( res2 )
	{
		*res2 = 0;
	}

	return ( DOSTRUE );
}

// --
