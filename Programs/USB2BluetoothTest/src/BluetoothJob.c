
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "BluetoothWindow.h"

// --

static void
ScanForPeers( struct BTWindowJob *job, struct IORequest *request )
{
	struct Bluetooth2IFace *bt2;
	struct BTControllerLocation location;
	struct BTInquiryResult inquiry;
	struct BTHCIError error;

	bt2 = (APTR) GetInterface( (APTR) request->io_Device, BLUETOOTH2_INTERFACE, 3, NULL );

	if ( ! bt2 )
	{
		job->DiscoveryResult = BT_HCI_UNSUPPORTED;

		return;
	}

	inquiry.Devices = job->Peers;
	inquiry.Capacity = BT_WINDOW_PEERS;
	inquiry.Count = 0;

	job->DiscoveryResult = bt2->DiscoverController( job->Unit, & location, & inquiry, & error );

	DropInterface( (APTR) bt2 );

	job->Count = inquiry.Count;

	if ( job->DiscoveryResult == BT_HCI_OK )
	{
		job->Result = RETURN_OK;
	}
	else
	{
		ReportHCIError( job->DiscoveryResult, & error );
	}
}

// --

static LONG
BluetoothJob( void )
{
	struct BTWindowJob *job;
	struct IORequest *request;
	struct MsgPort *port;
	struct Task *task;

	LONG cancelled;

	task = FindTask( NULL );
	job = task->tc_UserData;

	ObtainSemaphore( & job->Lock );

	cancelled = job->Cancelled;
	job->Worker = task;

	ReleaseSemaphore( & job->Lock );

	if ( ! cancelled )
	{
		port = AllocSysObjectTags( ASOT_PORT, TAG_END );

		if ( port )
		{
			request = AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( *request ),
				ASOIOR_ReplyPort, port, TAG_END );

			if ( request )
			{
				if ( ! OpenDevice( BLUETOOTH2_NAME, 0, request, 0 ) )
				{
					if ( job->Action == BT_WINDOW_SCAN )
					{
						ScanForPeers( job, request );
					}
					else if ( job->Action == BT_WINDOW_PLAY )
					{
						job->Result = PlayBluetoothAudio( request, job->Unit, job->Address, job->Path );
					}
					else
					{
						job->Result = PushObjectFile( request, job->Unit, job->Address, job->Channel, job->Path );
					}

					CloseDevice( request );
				}

				FreeSysObject( ASOT_IOREQUEST, request );
			}

			FreeSysObject( ASOT_PORT, port );
		}
	}

	ObtainSemaphore( & job->Lock );

	job->Worker = NULL;

	ReleaseSemaphore( & job->Lock );

	return ( RETURN_OK );
}

// --

LONG
StartBluetoothJob( struct BTWindowJob *job, struct MsgPort *reply )
{
	struct Process *process;

	BPTR output;

	job->Count = 0;
	job->Cancelled = FALSE;
	job->Worker = NULL;
	job->Result = RETURN_FAIL;
	job->DiscoveryResult = BT_HCI_NO_CONTROLLER;
	job->Death.dm_Msg.mn_Node.ln_Type = NT_MESSAGE;
	job->Death.dm_Msg.mn_ReplyPort = reply;
	job->Death.dm_Msg.mn_Length = sizeof( job->Death );

	output = Output();

	process = CreateNewProcTags( NP_Name, "USB2 Bluetooth operation", NP_Entry, BluetoothJob,
		NP_UserData, job, NP_Child, FALSE, NP_Output, output, NP_CloseOutput, FALSE, NP_StackSize, 65536,
		NP_NotifyOnDeathMessage, & job->Death, TAG_END );

	return ( process != NULL );
}

// --

void
CancelBluetoothJob( struct BTWindowJob *job )
{
	ObtainSemaphore( & job->Lock );

	job->Cancelled = TRUE;

	if ( job->Worker )
	{
		Signal( job->Worker, SIGBREAKF_CTRL_C );
	}

	ReleaseSemaphore( & job->Lock );
}

// --
