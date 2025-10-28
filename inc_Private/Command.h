
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

#ifndef INC_PRIVATE_USB2_ALL_H
#error Include "usb2_all.h" first
#endif

#ifndef INC_PRIVATE_COMMAND_H
#define INC_PRIVATE_COMMAND_H

/***************************************************************************/

enum USB2_Cmd
{
	CMD_Unset = 0,

	// -- Start new Task/Process
	CMD_START_PROCESS,
	CMD_STOP_PROCESS,

	// -- Function Message
	CMD_FKT_DETACH,			// Send Detach msg too all Registers
};

// --

struct CmdHeader
{
	struct Message		cm_Message;
	enum USB2_Cmd		cm_Command;
	struct USBBase *	cm_usbbase;
	U32					cm_Result;
	struct Task *		cm_Task;
	U32					cm_Mask;
};

// --

struct FktMessage
{
	struct CmdHeader			fm_Cmd;
	struct RealFunctionNode *	fm_FN;
};

// --

/***************************************************************************/

#endif // INC_PRIVATE_COMMAND_H
