
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "Master.h"

// --

SEC_CODE static void
Master_ReplyStart( struct ExecIFace *IExec, struct MasterStartMsg *msg, S32 result )
{
	IExec->Disable();

	msg->Result = result;

	IExec->Signal( msg->Parent, msg->Mask );

	IExec->Enable();
}

// --
