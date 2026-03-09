
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

struct CommandMessage
{
	struct Message				cm_Message;
	U32							cm_Command;
	// uint16						cm_Sender;
	// U32			            cm_Result;
	// struct CameraNode *         cm_Camera;

	// union
	// {
//--	    struct MsgPort *        CommandPort;
		// struct StorageNode *    StorageNode;
		// struct FileNode *		FileNode;
		// struct LogNode *		LogNode;
//--		STR					String;
		// U32					Value;
//--		APTR					Data;
    // }
	// cm_Parameter1;

	// union
	// {
	// 	struct FileNode *		FileNode;
	// 	STR					String;
	// 	U32					Value;
	// 	APTR					Data;
	// }
	// cm_Parameter2;

	// union
	// {
//--		BPTR					FileHandle;
//--		struct FileNode *		FileNode;
		// STR					String;
		// U32					Value;
//--		APTR					Data;
	// }
	// cm_Parameter3;

//--    union
//--	{
//--		STR					String;
//--		U32					Value;
//--		APTR					Data;
//--	}							
//--	cm_Parameter4;
//--
//--    union
//--    {
//--		STR					String;
//--		U32					Value;
//--		APTR					Data;
//--	}
//--	cm_Parameter5;
};

// Commands

// enum
// {
// 	// Common Commands

// 	CMD_Camera_Added,
// 	CMD_Camera_Removed,
// 	CMD_Camera_Storage_Added,
// 	CMD_Camera_Storage_Removed,
// 	CMD_Camera_File_Added,
// 	CMD_Camera_File_Removed,
// 	CMD_Camera_File_Transfer_Init,
// 	CMD_Camera_File_Transfer_Update,
// 	CMD_Camera_File_Transfer_Done,

// 	// File System Commands

// 	// Graphics Commands

// 	GFXCMD_Show_Graphics,
// 	GFXCMD_Hide_Graphics,
// 	GFXCMD_Transfer_Start,
// 	GFXCMD_Transfer_Abort,
// 	GFXCMD_Camera_Selector_Start,
// 	GFXCMD_Camera_Storage_Scan_Start,
// 	GFXCMD_Camera_Storage_Scan_Files,
// 	GFXCMD_Camera_Storage_Scan_End,
// 	GFXCMD_Camera_Thumb_Draw,
//--	GFXCMD_Close_Windows,
	// GFXCMD_Log_Add,
	// GFXCMD_Log_Update,

	// // USB Commands

//--	USBCMD_Transfer_End,
// 	USBCMD_Transfer_Abort,
// 	USBCMD_Camera_Object_Get,
// 	USBCMD_Camera_Thumb_Get,
// 	USBCMD_Camera_Thumb_CancelGet,

// 	// Master Commands

// 	MSTCMD_Log_Message,
// };

// --

#endif
