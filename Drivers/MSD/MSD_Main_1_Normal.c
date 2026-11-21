
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

SEC_CODE void MSD_Main_1_Normal( struct USBBase *usbbase, struct MSDDevice *msddev )
{
struct USB2_MsgPort tick_port;
struct TimeRequest tick;
struct MSDDisk *msddisk;
S32 tick_ready;
U32 wait;
U32 mask;

	TASK_NAME_ENTER( "MSD : MSD_Main_1_Normal" );
	USBERROR( "MSD : MSD_Main_1_Normal" );

	msddev->Running = TRUE;
	MEM_SET( & tick_port, 0, sizeof( tick_port ) );
	tick_ready = MSGPORT_INIT( & tick_port );

	if ( tick_ready )
	{
		MEM_COPY( usbbase->usb_TimeRequest, & tick, sizeof( tick ) );
		tick.Request.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
		tick.Request.io_Message.mn_ReplyPort = & tick_port.ump_MsgPort;
		tick.Request.io_Message.mn_Length = sizeof( tick );
		tick.Request.io_Command = TR_ADDREQUEST;
		tick.Time.Seconds = 2;
		tick.Time.Microseconds = 0;

		IO_SEND( & tick );
	}

	// -- Normal mode

	wait  = SIGBREAKF_CTRL_C;
	wait |=	msddev->msddev_Register->reg_Public.Stack_MsgPortBit;
	wait |= msddev->msddev_MsgPort_Abort.ump_Signal.sig_Signal_Mask;
	wait |= msddev->msddev_MsgPort_Begin.ump_Signal.sig_Signal_Mask;

	if ( tick_ready )
	{
		wait |= tick_port.ump_Signal.sig_Signal_Mask;
	}

	while( ( msddev->Running ) && ( ! msddev->msddev_Register->reg_Function->fkt_Detach ) )
	{
		mask = TASK_WAIT( wait );

		if ( mask & SIGBREAKF_CTRL_C )
		{
			USBDEBUG( "MSD : Got CTRL+C" );
		}

		if ( ( mask & SIGBREAKF_CTRL_C ) || ( msddev->msddev_Register->reg_Function->fkt_Detach ) )
		{
			msddev->Running = FALSE;
		}

		if ( ( msddev->Running ) && ( tick_ready ) && ( mask & tick_port.ump_Signal.sig_Signal_Mask ) )
		{
			if ( MSGPORT_GETMSG( & tick_port ) )
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
						MSD_Disk_MediaCheck( usbbase, msddisk );

						if ( ( msddisk->msddisk_MediaPresent ) && ( ! msddisk->msddisk_Announced ) )
						{
							msddisk->msddisk_Announced = usbbase->usb_IMounter->AnnounceDeviceTags( MSD_Device_Name, msddisk->msddisk_UnitNr,
								MNTA_DosNamePrefixHint, "USB",
								TAG_END
							) ? TRUE : FALSE;
						}
					}

					msddisk = (PTR) Node_Next( & msddisk->msddisk_Node );
				}

				tick.Time.Seconds = 2;
				tick.Time.Microseconds = 0;
				IO_SEND( & tick );
			}
		}

		if ( mask & msddev->msddev_MsgPort_Abort.ump_Signal.sig_Signal_Mask )
		{
			USBERROR( "MSD : __myHandle_Abort" );

			MSD_Main__myHandle_Abort( usbbase, msddev );
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
		IO_STOP( & tick );
		MSGPORT_FREE( & tick_port );
	}

	TASK_NAME_LEAVE();
}

#endif
// --
