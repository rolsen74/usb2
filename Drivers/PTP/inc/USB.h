
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
	struct USB2_DriverMessage *		us_StartMsg;
	struct USB2_Register *			us_Register;
	struct USB2_EPResource *		us_Res_BulkIn;
	struct USB2_EPResource *		us_Res_BulkOut;
	struct MsgPort *				us_CmdMsgPort;
	struct CameraInfoNode			us_CameraNode;
	U32								us_TransactionID;
	S32								us_Session;
	S32								us_Running;
	S32								us_Detached;
};

// --

#endif
