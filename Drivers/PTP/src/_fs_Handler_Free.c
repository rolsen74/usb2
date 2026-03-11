
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
		fs->fs_DeviceNode->dn_Port = NULL;
		fs->fs_DeviceNode = NULL;
	}

	if ( fs->fs_VolumeNode )
	{
		FreeDosEntry( (PTR) fs->fs_VolumeNode );
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

	if ( FS_CmdMsgPort )
	{
		FreeSysObject( ASOT_PORT, FS_CmdMsgPort );
		FS_CmdMsgPort = NULL;
	}

}

// --
