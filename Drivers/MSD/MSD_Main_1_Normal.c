
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"
#include "MSD.h"

// --
#if 1

SEC_CODE static void
MSD_Resume( struct USBBase *usbbase, PTR data, PTR unused UNUSED )
{
	struct USB2_DriverMessage *msg;
	struct MSDDevice *msddev;

	msddev = data;
	msg = msddev->msddev_StartMsg;

	MSD_Main( usbbase, msddev );

	MSD_Device_Free( usbbase, msddev );

	USB2_DriverWorkerResult( usbbase, msg, TASK_Return_Error );
}

// --

SEC_CODE static S32
MSD_Promote( struct USBBase *usbbase, struct MSDDevice *msddev, U32 mask )
{
	struct USB2_WorkerHandoff handoff;

	MEM_SET( &handoff, 0, sizeof( handoff ) );

	handoff.Entry = MSD_Resume;
	handoff.Data = msddev;
	handoff.Ports[0] = &msddev->msddev_MsgPort_Begin.ump_MsgPort;
	handoff.Ports[1] = &msddev->msddev_Register->reg_MsgPort.ump_MsgPort;
	handoff.Ports[2] = ( msddev->TickPort.ump_StructID == ID_USB2_MP ) ? &msddev->TickPort.ump_MsgPort : NULL;
	handoff.Endpoints[0] = msddev->msddev_Res_Control;
	handoff.Endpoints[1] = msddev->msddev_Res_Bulk_In;
	handoff.Endpoints[2] = msddev->msddev_Res_Bulk_Out;
	handoff.Pending = mask;

	return ( USB2_PromoteWorker( usbbase, &handoff ) );
}

// --

SEC_CODE S32
MSD_Main_1_Normal( struct USBBase *usbbase, struct MSDDevice *msddev )
{
	struct USB2_MsgPort *tick_port;
	struct TimeRequest *tick;
	struct MSDDisk *msddisk;

	S32 tick_ready;
	U32 wait;
	U32 mask;

	TASK_NAME_ENTER( "MSD : MSD_Main_1_Normal" );
	USBERROR( "MSD : MSD_Main_1_Normal" );

	tick_port = &msddev->TickPort;
	tick = &msddev->Tick;
	tick_ready = tick_port->ump_StructID == ID_USB2_MP;

	if ( ! msddev->Running )
	{
		msddev->Running = TRUE;

		tick_ready = MSGPORT_INIT( tick_port );

#ifdef MSD_DEBUG
		for ( msddisk = Header_Head( &msddev->msddev_MSDDisk_Header ); msddisk; msddisk = Node_Next( msddisk ) )
		{
			if ( ( MSD_TRACE_OPTICAL( msddisk ) ) && ( msddisk->msddisk_Removable ) )
			{
				MSDDEBUG( "MSD: interface %lu unit %lu LUN %lu media poll timer %s\n",
					msddev->msddev_InterfaceNr, msddisk->msddisk_UnitNr, msddisk->msddisk_Lun,
					( tick_ready ) ? "scheduled" : "reply port unavailable" );
			}
		}
#endif

		if ( tick_ready )
		{
			MEM_COPY( usbbase->usb_TimeRequest, tick, sizeof( *tick ) );
			tick->Request.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
			tick->Request.io_Message.mn_ReplyPort = & tick_port->ump_MsgPort;
			tick->Request.io_Message.mn_Length = sizeof( *tick );
			tick->Request.io_Command = TR_ADDREQUEST;
			tick->Time.Seconds = 2;
			tick->Time.Microseconds = 0;

			IO_SEND( tick );
		}
	}

	// -- Normal mode

	wait  = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_E;
	wait |=	msddev->msddev_Register->reg_Public.Stack_MsgPortBit;
	wait |= msddev->msddev_MsgPort_Begin.ump_Signal.sig_Signal_Mask;

	if ( tick_ready )
	{
		wait |= tick_port->ump_Signal.sig_Signal_Mask;
	}

	while( ( msddev->Running ) && ( ! msddev->msddev_Register->reg_Function->fkt_Detach ) )
	{
		mask = TASK_WAIT( wait );

		if ( ( mask & SIGBREAKF_CTRL_E ) && ( ! ( mask & SIGBREAKF_CTRL_C ) ) && ( MSD_Promote( usbbase, msddev, mask ) ) )
		{
			TASK_NAME_LEAVE();

			return ( TRUE );
		}

		if ( mask & SIGBREAKF_CTRL_C )
		{
			USBDEBUG( "MSD : Got CTRL+C" );
		}

		if ( ( mask & SIGBREAKF_CTRL_C ) || ( msddev->msddev_Register->reg_Function->fkt_Detach ) )
		{
			msddev->Running = FALSE;
		}

		if ( ( msddev->Running ) && ( tick_ready ) && ( mask & tick_port->ump_Signal.sig_Signal_Mask ) )
		{
			if ( MSGPORT_GETMSG( tick_port ) )
			{
				msddisk = (PTR) msddev->msddev_MSDDisk_Header.uh_Head;

				while( msddisk )
				{
					if ( msddev->msddev_Register->reg_Function->fkt_Detach )
					{
						MSD_Disk_Detach( usbbase, msddisk );
					}
					else if ( ( msddisk->msddisk_Removable ) && ( ! msddev->msddev_Detached ) )
					{
#ifdef MSD_DEBUG
						if ( ( MSD_TRACE_OPTICAL( msddisk ) ) && ( ! msddisk->msddisk_FirstTimedProbe ) )
						{
							msddisk->msddisk_FirstTimedProbe = TRUE;

							MSDDEBUG( "MSD: interface %lu unit %lu LUN %lu first timed media poll, timer error %ld\n",
								msddev->msddev_InterfaceNr, msddisk->msddisk_UnitNr,
								msddisk->msddisk_Lun, (S32) tick->Request.io_Error );
						}
#endif
						MSD_Disk_MediaCheck( usbbase, msddisk );

						if ( msddisk->msddisk_MediaPresent )
						{
							MSD_Disk_Announce( usbbase, msddisk );
						}
					}

					msddisk = (PTR) Node_Next( & msddisk->msddisk_Node );
				}

				tick->Time.Seconds = 2;
				tick->Time.Microseconds = 0;
				IO_SEND( tick );
			}
		}

		if ( mask & msddev->msddev_MsgPort_Begin.ump_Signal.sig_Signal_Mask )
		{
//			USBERROR( "MSD : __myHandle_Begin" );

			MSD_Main__myHandle_Begin( usbbase, msddev );
		}

		if ( mask & msddev->msddev_Register->reg_Public.Stack_MsgPortBit )
		{
			USBDEBUG( "MSD : Got Stack Msg" );

			MSD_Main__myHandle_Stack( usbbase, msddev );
		}
	}

	if ( tick_ready )
	{
		IO_STOP( tick );
		MSGPORT_FREE( tick_port );
	}

	TASK_NAME_LEAVE();

	return ( FALSE );
}

#endif
// --
