
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_ALL_H
#error Include "inc/All.h" first
#endif

#ifndef INC_FILESYSTEM_H
#define INC_FILESYSTEM_H

// --

#define FOLLOW_LINKS		TRUE
#define DONT_FOLLOW_LINKS	FALSE
#define LOCK_MODIFY			1

// --
// FS_ObjNode

struct FS_ObjNode
{
	struct Node				on_Node;
	STR						on_Name;
	U32						on_NameLen;
	struct FS_ObjNode *		on_Parent_dir;				/* parent node */
	// struct CameraNode *		Camera;

	// /*
	// **  For a directory; this is the list of files/dirs,
	// **  For a file; this is the list of data blocks.
	// */

	struct List				on_Content_List;

	/*
	**  If this node represents a hardlink, this points
	**  to the target object node the hardlink refers to.  Otherwise 0.
	*/

	struct FS_ObjNode *		on_HardLink_Target;		/* the object the hardlink points to. */

	// /*
	// **  If this is non-zero, this object has hard links to it.
	// **  This points to the first or next one in a chain.
	// **  Zero terminates the list.
	// */
	struct FS_ObjNode *		on_Next_HardLink;			/* the first or next hardlinked item  */

	S32						on_Type;					/* file, directory, link */

	int64					on_File_Size;

	U32						on_Protection;
	U32						on_Owner;
	U32						on_Group;
//	S32						on_Dying;

	struct DateStamp		on_Date;
	STR						on_Comment;
	// struct CommandMessage *	DeadMessage;
};

// --
// FS_ObjLock

struct FS_ObjLock
{
	struct Lock				Lock;				/* fl_Key is ptr to the object node as fake "inode" */
	struct FS_ObjNode *		Node;				/* corresponding node. No, we don't want to rely on fl_Key. */

	struct FS_DataBlock *	Current_Block;		/* current block in the nodes block_list */
//    S32				      current_block_offset;   /* offset within block */
//    S32				      current_position;	      /* position within file */

	U32						Flags;				/* sundry flags */

    // struct FileHandle *     Handle;			    /* so we can find it from the lock to shutdown. */
};

// --

struct FS_Struct
{
	struct FileSystemVectorPort *	fs_FSVector_Port;
	U32						fs_FSVector_PortBit;

	struct DeviceNode *		fs_DeviceNode;
	struct FS_ObjNode *		fs_RootNode;

//	struct MsgPort *		fs_CmdMsgPort;
	U32						fs_CmdMsgPortBit;

	S32						fs_VolumeAdded;
	struct VolumeNode *		fs_VolumeNode;

	BPTR					fs_LockList;

	S32						fs_Running;
	S32						fs_WriteProtected;
	U64						fs_NumBytesUsed;

	struct List				fs_CameraList;
	struct SignalSemaphore	fs_Semaphore;
};

#if 0

/* These are defined in dos.h  */

#define MAX_FILENAME                MAX_VP_FILENAME
#define MAX_COMMENT                 MAX_VP_COMMENT
#define MAX_SOFTLINK_TARGET_LEN     MAX_VP_SOFTLINK_TARGET

#define LOCK_MODIFY 			    1

/*[ PTP FSStruct ]**********************************************************/

struct FS_Struct
{
	uint8					    fs_Running;
	uint8					    fs_WriteProtected;
	uint8					    fs_Pad1;
	uint8					    fs_Pad2;

	struct FileSystemVectorPort*fs_FSVectorPort;
	U32				        fs_FSVectorPortBit;

	struct DeviceNode *		    fs_DeviceNode;

	int					    fs_VolumeAdded;
	struct VolumeNode *		    fs_VolumeNode;

	BPTR						fs_LockList;
	struct SignalSemaphore	    fs_Semaphore;

// --    U32				     fs_WriteProtectKey;

	uint64					    fs_NumBytesUsed;

	struct FS_ObjNode *		    fs_RootNode;

	struct MsgPort *			fs_PublicMP;
	U32						fs_PublicBit;
// -- 	  U32					  fs_FSMsgPortBit;

// --    struct DosList *	     fs_DosList;
// --
// --    struct List *		     fs_FileHandles;
// --    U32				     fs_FileHandlesCount;
// --
// --    struct MsgPort *	     fs_IEMsgPort;
// --    struct IOStdReq *	     fs_IEIOReq;
// --    struct InputEvent *     fs_IEData;
// --    U32				     fs_IEStat;
// --
// --    U32				     fs_InUse;
// --
// --    struct FS_FileObject *  fs_RootParentObject;
// --    struct FS_FileObject *  fs_RootObject;
// --
// --    STR		     fs_VolumeName;
// --
// --    U32				     fs_FileSize;
};


/*[ FS_DataBlock ]**********************************************************/

/* the datablock meta-data minus the data mem (@*data) */

// -- struct FS_DataBlock
// -- {
// -- 	  U32 a;
// -- //    struct MinNode          MinNode;		    /* must be first in struct */
// -- //
// -- //    int16                   shortened;		    /* TRUE if this block has been shortened
// -- //											       and stores just as much data as necessary */
// -- //    S32                   num_data_bytes;	    /* number of bytes in this block which hold data */
// -- //
// -- //    #if defined(USE_BLOCK_CHECKSUMS)
// -- //    U32                  checksum;		    /* to track storage errors (see fswrite.c) */
// -- //    #endif /* USE_BLOCK_CHECKSUMS */
// -- //
// -- //    uint8 *				    data;		       /* shared mem (swappable) */
// -- };

/*[ Protos ]****************************************************************/

U32				PTP_FSHandler(						void );

S32 			    FS_Compare_Names( 			    	STR a, STR b );

S32 			    FS_Free_Lock( 						struct FS_Struct *fs, struct FS_ObjLock *lock );
struct FS_ObjNode * FS_Locate_Node_by_Rellockstr(		struct FS_Struct *fs, S32 *res2, struct FS_ObjNode **parent, struct FS_ObjLock *lock, STR name, S32 follow_links );
struct FS_ObjLock * FS_New_Lock_by_Node(				struct FS_Struct *fs, S32 *res2, struct FS_ObjNode *objnode, S32 access );
void			    FS_Recursive_Change_Update(			struct FS_Struct *fs, struct FS_ObjNode *node );

S32			    FS__FSDeviceInfoData(				struct FSVP *vp, S32 *res2, struct InfoData *id );
struct Lock *	    FS__FSDupLock( 						struct FSVP *vp, S32 *res2, struct Lock *lockin );
struct Lock *	    FS__FSDupLockFromFH( 				struct FSVP *vp, S32 *res2, struct FileHandle *file );
S32			    FS__FSExamineDir(					struct FSVP *vp, S32 *res2, struct PRIVATE_ExamineDirContext *ctx );
struct ExamineData *FS__FSExamineFile( 					struct FSVP *vp, S32 *res2, struct FileHandle *fh );
struct ExamineData *FS__FSExamineLock( 					struct FSVP *vp, S32 *res2, struct Lock *objLock );
struct Lock *	    FS__FSLock(							struct FSVP *vp, S32 *res2, struct Lock *rel_lock, STR name, S32 mode );
struct Lock *	    FS__FSParentDir(					struct FSVP *vp, S32 *res2, struct Lock *dirlock );
struct Lock *	    FS__FSParentOfFH( 					struct FSVP *vp, S32 *res2, struct FileHandle *file );
S32 			    FS__FSUnLock( 						struct FSVP *vp, S32 *res2, struct Lock *lock );
S32 			    FS__FSVolumeInfoData( 				struct FSVP *vp, S32 *res2, struct InfoData *info );

#endif


// --

#endif
