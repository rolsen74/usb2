
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

int32 _fs__FSDeviceInfoData( struct FSVP *vp, int32 *res2, struct InfoData *id )
{
struct PTP_FSStruct *fs;
U32 num_blocks;

//	MYINFO( "PTP-FS : _fs__FSDeviceInfoData" );

	fs = vp->FSV.FSPrivate;

	ObtainSemaphore( & fs->fs_Semaphore );

//	id->id_NumSoftErrors	= 0;	// now Reserved
//	id->id_UnitNumber		= 0;	// now Reserved
	id->id_DiskState		= ( fs->fs_WriteProtected ) ? ID_DISKSTATE_WRITE_PROTECTED : ID_DISKSTATE_VALIDATED;
//	id->id_NumBlocks		= 0;		// Number of Blocks (total)
//	id->id_NumBlocksUsed	= 0;		// Number of Blocks used
	id->id_BytesPerBlock	= 512;
	id->id_DOSType			= fs->fs_VolumeNode->vn_DiskType;
	id->id_VolumeNode		= MKBADDR( fs->fs_VolumeNode );
	id->id_UseCount			= 0;		// 0 = Not in use

	/*
	** Round up the amount of data stored. If there is nothing stored,
	** make sure that we don't return 0 since some software is completely
	** unprepared to handle it.
	*/

	num_blocks				= ( fs->fs_NumBytesUsed + ( id->id_BytesPerBlock - 1 )) / id->id_BytesPerBlock;

	if ( ! num_blocks )
	{
		num_blocks++;
	}

	id->id_NumBlocks		= num_blocks;
	id->id_NumBlocksUsed	= num_blocks;		/* RAM is always 100% full */

	if ( res2 )
	{
		*res2 = 0;
	}

	ReleaseSemaphore( & fs->fs_Semaphore );

	return(	DOSTRUE );
}

// --

