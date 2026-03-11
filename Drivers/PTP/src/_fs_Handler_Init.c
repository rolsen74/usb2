
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

/*[ Vectors ]***************************************************************/

static struct FileSystemVectors FSVectors =
{
/* StructSize			*/ sizeof( struct FileSystemVectors ),
/* Version				*/ FS_VECTORPORT_VERSION,
/* FSPrivate			*/ 0,
/* Reserved[3]			*/ { 0, 0, 0 },
/* DOSPrivate			*/ 0,

/* DOSEmulatePacket()	*/ NULL,

/* FSLock				*/ _fs__FSLock,					//  8 : ACTION_LOCATE_OBJECT
/* FSUnLock				*/ _fs__FSUnLock,				// 15 : ACTION_FREE_LOCK
/* FSDupLock			*/ _fs__FSDupLock,				// 19 : ACTION_COPY_LOCK
/* FSCreateDir			*/ NULL,
/* FSParentDir			*/ _fs__FSParentDir,			// 29 : ACTION_PARENT
/* FSDupLockFromFH		*/ _fs__FSDupLockFromFH,		// 19 : ACTION_COPY_LOCK_FH
/* FSOpenFromLock		*/ NULL,
/* FSParentOfFH			*/ _fs__FSParentOfFH,			// 1031 : ACTION_PARENT_FH
/* FSOpen				*/ NULL,						// 1004 ACTION_FINDUPDATE : 1005 ACTION_FINDINPUT : 1006 ACTION_FINDOUTPUT
/* FSClose				*/ NULL,
/* FSDelete				*/ NULL,
/* FSRead				*/ NULL, // PTP_FSRead,
/* FSWrite				*/ NULL,
/* FSFlush				*/ NULL,

/* FSChangeFilePosition	*/ NULL,
/* FSChangeFileSize		*/ NULL,

/* FSGetFilePosition	*/ NULL,
/* FSGetFileSize		*/ NULL,

/* FSChangeLockMode		*/ NULL,
/* FSChangeFileMode		*/ NULL,

/* FSSetDate			*/ NULL,
/* FSSetProtection		*/ NULL,						// 21 : ACTION_SET_PROTECT
/* FSSetComment			*/ NULL,
/* FSSetGroup			*/ NULL,
/* FSSetOwner			*/ NULL,
/* FSRename				*/ NULL,

/* FSCreateSoftLink		*/ NULL,
/* FSCreateHardLink		*/ NULL,
/* FSReadSoftLink		*/ NULL,

/* FSSameLock			*/ NULL,
/* FSSameFile			*/ NULL,
/* FSFileSystemAttr		*/ NULL,
/* FSVolumeInfoData		*/ _fs__FSVolumeInfoData,		// 26 : ACTION_INFO
/* FSDeviceInfoData		*/ _fs__FSDeviceInfoData,		// 25 : ACTION_HANDLER_INFO

/* FSGetFSSMData		*/ NULL,
/* FSFreeFSSMData		*/ NULL,

/* FSExamineLock		*/ _fs__FSExamineLock,			// 3030 : ACTION_EXAMINEDATA
/* FSExamineFile		*/ _fs__FSExamineFile,			// 3031 : ACTION_EXAMINEDATA_FH
/* FSExamineDir			*/ _fs__FSExamineDir,			// 3040 : ACTION_EXAMINEDATA_DIR
/* FSInhibit			*/ NULL,
/* FSWriteProtect		*/ NULL,
/* FSFormat				*/ NULL, // PTP__FSFormat,
/* FSSerialize			*/ NULL,
/* FSRelabel			*/ NULL,

/* reserved				*/ NULL,

/* FSAddNotify			*/ NULL,
/* FSRemoveNotify		*/ NULL,

/* FSLockRecord			*/ NULL,
/* FSUnLockRecord		*/ NULL,

/* V53_End_Marker		*/ -1
};

// --

S32 _fs_Handler_Init( struct FS_Struct *fs )
{
S32 retval;
char name[32];

	retval = FALSE;

	// --

	FS_CmdMsgPort = AllocSysObjectTags( ASOT_PORT,
		TAG_END
	);

	if ( ! FS_CmdMsgPort )
	{
		MYERROR( "PTP-FS : Error Allocating MsgPort" );
		goto bailout;
	}

	fs->fs_CmdMsgPortBit = 1UL << FS_CmdMsgPort->mp_SigBit;

	// --

	CopyStringBSTRToC( fs->fs_DeviceNode->dn_Name, name, sizeof( name ));

	/*
	** Create the rootdir node and initialise it.
	*/

	fs->fs_RootNode = _fs_Node_CreateDir( fs, NULL, name );

	if ( ! fs->fs_RootNode )
	{
		MYERROR( "PTP-FS : Cannot create root dir" );
		goto bailout;
	}

	fs->fs_FSVector_Port = AllocDosObjectTags( DOS_FSVECTORPORT,
		ADO_Vectors, FSVectors,
		ADO_Name, name,
		TAG_END
	);

	if ( ! fs->fs_FSVector_Port )
	{
		MYERROR( "PTP-FS : Error allocating FSVectorPort" );
		goto bailout;
	}

	fs->fs_FSVector_PortBit = 1UL << fs->fs_FSVector_Port->MP.mp_SigBit;
	fs->fs_FSVector_Port->FSV.FSPrivate = fs;

	/*
	** Initialise the device node port so we can be found by dos.library.
	** The instant we reply to the ACTION_STARTUP packet, we will be accessed.
	*/

	fs->fs_DeviceNode->dn_Port = & fs->fs_FSVector_Port->MP;

	/*
	** Allocate volume node.
	*/

	fs->fs_VolumeNode = (PTR) MakeDosEntry( "PTP", DLT_VOLUME );

	if ( ! fs->fs_VolumeNode )
	{
		MYERROR( "PTP-FS : Error allocating VolumeNode" );
		goto bailout;
	}

	fs->fs_VolumeNode->vn_Port		= &fs->fs_FSVector_Port->MP;
	fs->fs_VolumeNode->vn_DiskType	= ID_VP255_DOS_DISK;

	DateStamp( &fs->fs_VolumeNode->vn_VolumeDate );

	// ---

	if ( ! NonBlockingModifyDosEntry( (PTR) fs->fs_VolumeNode, NBM_ADDDOSENTRY, 0, 0 ))
	{
		MYERROR( "PTP-FS : Error adding Volume Node" );
		goto bailout;
	}

	fs->fs_VolumeAdded = TRUE;

	NonBlockingModifyDosEntry( 0, NBM_DISKINSERTED,  0, 0 );

	// ---

	retval = TRUE;

bailout:

	return( retval );
}

