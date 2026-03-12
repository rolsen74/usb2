
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

#ifndef INC_USB_H
#define INC_USB_H

// --

enum CmdType
{
	CMDTYPE_Cmd_None_Stat,
	CMDTYPE_Cmd_Read_Stat,
	CMDTYPE_Cmd_Send_Stat,
};

enum CmdStage
{
	CMDSTAGE_Send_Header1,
	CMDSTAGE_Send_Header2,

	CMDSTAGE_Read_Data1,
	CMDSTAGE_Read_Data2,
	CMDSTAGE_Read_Data3,

	CMDSTAGE_Send_Data1,
	CMDSTAGE_Send_Data2,

	CMDSTAGE_Read_Responce1,
	CMDSTAGE_Read_Responce2,
};

struct USB_Struct;

struct USB_Command
{
	struct Node						ucmd_Node;
	enum CmdType					ucmd_Type;
	enum CmdStage					ucmd_State;
	U32								ucmd_Command;
	struct PTP_Container			ucmd_Container;
	U32								ucmd_Data_Length;
	U8 *							ucmd_Data_Buffer;
	U32								ucmd_Data_Current;
	void							( *ucmd_Next )( struct USB_Struct *, struct USB_Command * );
	void							( *ucmd_Function )( struct USB_Struct *, struct USB_Command * );
};

// --

struct USB_Struct
{
	S32								us_Detached;
	S32								us_Running;
	U32								us_TransactionID;
	U32								us_SessionID;
	struct USB2_Register *			us_Register;
	struct USB2_DriverMessage *		us_StartMsg;
	struct USB2_EPResource *		us_Res_BulkIn;
	struct USB2_EPResource *		us_Res_BulkOut;
	struct USB2_EPResource *		us_Res_Control;
	struct USB2_EPResource *		us_Res_Interrupt;
	struct List						us_CmdList;
	S32								us_CmdListCnt;
	struct MsgPort *				us_CmdMsgPort;
	struct CameraInfoNode			us_CameraNode;
	struct List						us_USBList;
	struct USB_Command *			us_USBActive;
	U32								us_Errors_Int;
	U32								us_Errors_Bulk;
};

// --

#endif
