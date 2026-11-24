
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

SEC_CODE void MSD_Main__myHandle_Begin( struct USBBase *usbbase, struct MSDDevice *msddev )
{
struct IOStdReq *ioreq;
S32 reply;

	TASK_NAME_ENTER( "MSD : __myHandle_Begin" );
//	USBERROR( "MSD : __myHandle_Begin" );

	while( TRUE )
	{
		SEMAPHORE_OBTAIN( &msddev->msddev_MSDBase->msdbase_MSDDisk_Semaphore );

		ioreq = MSGPORT_GETMSG( &msddev->msddev_MsgPort_Begin );

		SEMAPHORE_RELEASE( &msddev->msddev_MSDBase->msdbase_MSDDisk_Semaphore );

		if ( ! ioreq )
		{
			// No more IORequests.. not an error
			break;
		}

		if ( ( ! msddev->Running ) || ( msddev->msddev_Detached ) || ( msddev->msddev_Register->reg_Function->fkt_Detach ) )
		{
			ioreq->io_Actual = 0;
			ioreq->io_Error = TDERR_DiskChanged;

			MSGPORT_REPLYMSG( ioreq );

			continue;
		}

		switch( ioreq->io_Command )
		{
			case /* 0002 */ CMD_READ:					reply = MSD_Cmd_0002_CMD_Read( usbbase, msddev, (PTR) ioreq ); break;
			case /* 0003 */ CMD_WRITE:					reply = MSD_Cmd_0003_CMD_Write( usbbase, msddev, (PTR) ioreq ); break;
			case /* 0009 */ TD_MOTOR:					reply = MSD_Cmd_0009_TD_Motor( usbbase, msddev, (PTR) ioreq ); break;
			case /* 000A */ TD_SEEK:					reply = MSD_Cmd_000A_TD_Seek( usbbase, msddev, (PTR) ioreq ); break;
			case /* 000B */ TD_FORMAT:					reply = MSD_Cmd_000B_TD_Format( usbbase, msddev, (PTR) ioreq ); break;
			case /* 000D */ TD_CHANGENUM:				reply = MSD_Cmd_000D_TD_ChangeNum( usbbase, msddev, (PTR) ioreq ); break;
			case /* 000E */ TD_CHANGESTATE:				reply = MSD_Cmd_000E_TD_ChangeState( usbbase, msddev, (PTR) ioreq ); break;
			case /* 000F */ TD_PROTSTATUS:				reply = MSD_Cmd_000F_TD_ProtStatus( usbbase, msddev, (PTR) ioreq ); break;
			case /* 0012 */ TD_GETDRIVETYPE:			reply = MSD_Cmd_0012_TD_GetDriveType( usbbase, msddev, (PTR) ioreq ); break;
			case /* 0016 */ TD_GETGEOMETRY:				reply = MSD_Cmd_0016_TD_GetGeometry( usbbase, msddev, (PTR) ioreq ); break;
			case /* 0017 */ TD_EJECT:					reply = MSD_Cmd_0017_TD_Eject( usbbase, msddev, (PTR) ioreq ); break;
			case /* 0018 */ TD_READ64:					reply = MSD_Cmd_0018_TD_Read64( usbbase, msddev, (PTR) ioreq ); break;
			case /* 0019 */ TD_WRITE64:					reply = MSD_Cmd_0019_TD_Write64( usbbase, msddev, (PTR) ioreq ); break;
			case /* 001A */ TD_SEEK64:					reply = MSD_Cmd_001A_TD_Seek64( usbbase, msddev, (PTR) ioreq ); break;
			case /* 001B */ TD_FORMAT64:				reply = MSD_Cmd_001B_TD_Format64( usbbase, msddev, (PTR) ioreq ); break;
			case /* 001C */ HD_SCSICMD:					reply = MSD_Cmd_001C_HD_SCSICmd( usbbase, msddev, (PTR) ioreq ); break;
			case /* 4000 */ NSCMD_DEVICEQUERY:			reply = MSD_Cmd_4000_NSCmd_DeviceQuery( usbbase, msddev, (PTR) ioreq ); break;
			case /* A004 */ NSCMD_TD_GETGEOMETRY64:		reply = MSD_Cmd_A004_NSCmd_GetGeometry64( usbbase, msddev, (PTR) ioreq ); break;
			case /* A005 */ NSCMD_TD_CHANGEUNIT:		reply = MSD_Cmd_A005_NSCmd_ChangeUnit( usbbase, msddev, (PTR) ioreq ); break;
			case /* C000 */ NSCMD_TD_READ64:			reply = MSD_Cmd_C000_NSCmd_Read64( usbbase, msddev, (PTR) ioreq ); break;
			case /* C001 */ NSCMD_TD_WRITE64:			reply = MSD_Cmd_0019_TD_Write64( usbbase, msddev, (PTR) ioreq ); break;
			case /* C002 */ NSCMD_TD_SEEK64:			reply = MSD_Cmd_001A_TD_Seek64( usbbase, msddev, (PTR) ioreq ); break;
			case /* C003 */ NSCMD_TD_FORMAT64:		reply = MSD_Cmd_001B_TD_Format64( usbbase, msddev, (PTR) ioreq ); break;

			case /* 0004 */ CMD_UPDATE:				reply = MSD_Cmd_0004_CMD_Update( usbbase, msddev, (PTR) ioreq ); break;

			case /* 0005 */ CMD_CLEAR:
			{
				USBERROR( "MSD : _cmd_0005_CMD_CLEAR" );
				reply = TRUE;
				break;
			}

			default:
			{
				USBERROR( "MSD : __myHandle_Begin : Unknown :  IOReq %p : Command $%04lx", ioreq, ioreq->io_Command );
				ioreq->io_Error = IOERR_NOCMD;
				reply = TRUE;
				break;
			}
		}

		if ( reply )
		{
#ifdef MSD_DEBUG
			switch( ioreq->io_Command )
			{
				case CMD_READ:
				case TD_READ64:
				case NSCMD_TD_READ64:
				case CMD_WRITE:
				case TD_WRITE64:
				case NSCMD_TD_WRITE64:
				{
					struct MSDDisk *msddisk;

					msddisk = (PTR) ioreq->io_Unit;

					if ( ( msddisk ) && ( ioreq->io_Error ) )
					{
						MSDDEBUG( "MSD: unit %lu disk I/O command $%04lX failed, error %ld, requested %lu bytes\n",
							msddisk->msddisk_UnitNr, (U32) ioreq->io_Command,
							(S32) ioreq->io_Error, ioreq->io_Length );
					}
					else if ( ( msddisk ) && ( ! msddisk->msddisk_ReadLogged ) &&
						( ( ioreq->io_Command == CMD_READ ) || ( ioreq->io_Command == TD_READ64 ) ||
							( ioreq->io_Command == NSCMD_TD_READ64 ) ) && ( ioreq->io_Actual ) )
					{
						msddisk->msddisk_ReadLogged = TRUE;

						MSDDEBUG( "MSD: unit %lu first block read, %lu/%lu bytes\n",
							msddisk->msddisk_UnitNr, ioreq->io_Actual, ioreq->io_Length );
					}

					break;
				}
			}
#endif
			MSD_Disk_NotifyStatCallbacks( usbbase, ioreq );

			MSGPORT_REPLYMSG( ioreq );
		}
	}

	TASK_NAME_LEAVE();
}

#endif
// --
