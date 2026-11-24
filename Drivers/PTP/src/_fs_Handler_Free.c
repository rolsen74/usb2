
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void _fs_Handler_Free( struct FS_Struct *fs )
{
	if ( fs->fs_DeviceNode )
	{
		if ( ( fs->fs_FSVector_Port ) && ( fs->fs_DeviceNode->dn_Port == &fs->fs_FSVector_Port->MP ) )
		{
			fs->fs_DeviceNode->dn_Port = NULL;
		}

		fs->fs_DeviceNode = NULL;
	}

	if ( fs->fs_VolumeNode )
	{
		if ( fs->fs_VolumeAdded )
		{
			NonBlockingModifyDosEntry( (PTR)fs->fs_VolumeNode, NBM_REMFREEDOSENTRY, 0, 0 );
		}
		else
		{
			FreeDosEntry( (PTR)fs->fs_VolumeNode );
		}
		fs->fs_VolumeNode = NULL;
	}

	if ( fs->fs_FSVector_Port )
	{
		FreeDosObject( DOS_FSVECTORPORT, fs->fs_FSVector_Port );
		fs->fs_FSVector_Port = NULL;
	}

	if ( fs->fs_RootNode )
	{
		_fs_Node_Delete( fs, fs->fs_RootNode );
		fs->fs_RootNode = NULL;
	}

	if ( fs->fs_CmdMsgPort )
	{
		FreeSysObject( ASOT_PORT, fs->fs_CmdMsgPort );
		fs->fs_CmdMsgPort = NULL;
	}

}

// --
