
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
	struct MsgPort *				us_CmdMsgPort;
	struct CameraInfoNode			us_CameraNode;
};

// --

#endif
