
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

#ifndef INC_COMMAND_H
#define INC_COMMAND_H

// --

struct CameraInfoNode
{
	struct MsgPort *		cin_MsgPort;
	struct SignalSemaphore	cin_Gate;
	struct FS_ObjNode *		cin_DirNode;
	USB2_ID					cin_ID;
	U32						cin_StorageGeneration;
	S32						cin_Apple;
	S32						cin_Online;
	S32						cin_NoPartial;
	S32						cin_CanUpload;
	S32						cin_CanReadStorageInfo;
	char					cin_Name[127];	// File-system-safe model name
};

enum
{
	CMD_Camera_Added,
	CMD_Camera_Removed,
	CMD_Camera_Transaction,
};

struct CommandMessage
{
	struct Message			cm_Message;
	struct CameraInfoNode *	cm_InfoNode;
	U32						cm_Command;
	U16						cm_Operation;
	U16						cm_Response;
	U32						cm_ParameterCount;
	U32						cm_Parameters[3];
	U8 *					cm_Data;
	U32						cm_Limit;
	U32						cm_Actual;
	U32						cm_Reply[3];
	U32						cm_ReplyCount;
	BPTR					cm_Output;
	BPTR					cm_Input;
	S32						cm_SendData;
	S32						cm_Allocate;
	S32						cm_WantData;
	S32					cm_Error;
};

// --

#endif
