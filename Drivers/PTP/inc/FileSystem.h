
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

#define PTP_METADATA_LIMIT ( 16U * 1024 * 1024 )
#define PTP_MAX_DEPTH 64
#define PTP_MAX_UPLOAD ( 0xffffffffU - 12 )

struct FS_ObjNode
{
	struct Node				on_Node;
	struct FS_ObjNode *		on_NameIndexNext;
	struct List				on_Content_List;
	struct FS_ObjNode *		on_Parent_dir;
	struct FS_ObjNode *		on_CameraRoot;
	struct FS_ObjNode *		on_StorageRoot;
	struct CameraInfoNode *	on_Camera;
	STR						on_Name;
	U32						on_NameLen;
	U32						on_Storage;
	U32						on_Handle;
	U32						on_Depth;
	U32						on_References;
	U32						on_StorageGeneration;
	S32						on_StorageSeen;
	S32						on_StorageNew;
	U32						on_StorageChecked;
	S32						on_StorageRemoved;
	S32						on_Loaded;
	S32						on_Pending;
	S32						on_Type;
	S64						on_File_Size;
	U32						on_Protection;
	struct DateStamp		on_Date;
};

struct FS_ObjLock
{
	struct Lock				Lock;
	struct FS_ObjNode *		Node;
	U64						Position;
	BPTR					Cache;
	STR						CacheName;
	S32					WriteError;
};

struct FS_Struct
{
	struct FileSystemVectorPort *	fs_FSVector_Port;
	U32								fs_FSVector_PortBit;
	struct DeviceNode *				fs_DeviceNode;
	struct FS_ObjNode *				fs_RootNode;
	U32								fs_CmdMsgPortBit;
	struct MsgPort *				fs_CmdMsgPort;
	struct Message *				fs_ShutdownMsg;
	S32								fs_VolumeAdded;
	struct VolumeNode *				fs_VolumeNode;
	BPTR							fs_LockList;
	S32								fs_Running;
	struct SignalSemaphore			fs_Semaphore;
};

#endif
