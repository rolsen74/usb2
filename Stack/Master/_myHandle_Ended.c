
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --
// This Subtask have been Shutdown, now handle stuff

SEC_CODE static void
RetireStopMessage( struct USB2_TaskMsg *tm, struct USBBase *usbbase, struct intern *in )
{
	if ( tm->tm_TimeReq_Active )
	{
		IO_STOP( &tm->tm_TimeRequest );

		tm->tm_TimeReq_Active = FALSE;
	}

	NODE_REMNODE( &in->Shutdown_TM_List, tm );

	TASKMSG_FREE( tm );
}

// --

SEC_CODE static void
myHandle_Task_Ended( struct USBBase *usbbase, struct intern *in )
{
	struct USB2_TaskNode *tn;
	struct USB2_TaskMsg *tm;
	struct USB2_TaskMsg *next;

	for( tm = in->Shutdown_TM_List.uh_Head; tm; tm = next )
	{
		next = Node_Next( tm );
		tn = tm->tm_Task;

		if ( ( ! tn ) || ( tn->tn_State == TASK_State_Stopped ) )
		{
			RetireStopMessage( tm, usbbase, in );
		}
	}
}

// --
