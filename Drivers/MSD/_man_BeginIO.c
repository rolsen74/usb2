
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

SEC_CODE void MSD_Manager_BeginIO( struct DeviceManagerInterface *Self, struct IOStdReq *ioreq )
{
struct USBBase *usbbase;	// USB Device
struct MSDBase *MSDBase;	// MSD Device
struct MSDDisk *msddisk;

S32 queued;

	TASK_NAME_ENTER( "MSD : MSD_Manager_BeginIO" );
//	USBERROR( "MSD : MSD_Manager_BeginIO" );

	MSDBase = (PTR) Self->Data.LibBase;
	usbbase = MSDBase->msdbase_usbbase;
	msddisk	= (PTR) ioreq->io_Unit;

	// TD_REMCHANGEINT reuses the request held by TD_ADDCHANGEINT.
	if ( ioreq->io_Command == TD_REMCHANGEINT )
	{
		MSD_Cmd_0015_TD_RemChangeInt( usbbase, ioreq );

		if ( ioreq->io_Flags & IOF_QUICK )
		{
			ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
		}
		else
		{
			MSGPORT_REPLYMSG( ioreq );
		}

		TASK_NAME_LEAVE();

		return;
	}

	if ( ioreq->io_Command == NSCMD_TD_REMSTATCALLBACK )
	{
		MSD_Cmd_A007_NSCmd_RemStatCallBack( usbbase, ioreq );

		TASK_NAME_LEAVE();

		return;
	}

	ioreq->io_Message.mn_Node.ln_Type = NT_MESSAGE;
	ioreq->io_Flags		&= ~IOF_QUICK;
	ioreq->io_Error		= 0;
//	ioreq->io_Actual	= 0;

	// There is always a USB Driver behind the Unit
	// so just pass the message to msgport
	switch( ioreq->io_Command )
	{
		case TD_ADDCHANGEINT:
		{
			if ( MSD_Cmd_0014_TD_AddChangeInt( usbbase, ioreq ) )
			{
				MSGPORT_REPLYMSG( ioreq );
			}

			break;
		}

		// These commands manage held requests directly, including reuse of the
		// original ADD request for REMOVE.
		case NSCMD_TD_ADDSTATCALLBACK:
		{
			if ( MSD_Cmd_A006_NSCmd_AddStatCallBack( usbbase, ioreq ) )
			{
				MSGPORT_REPLYMSG( ioreq );
			}

			break;
		}


		// case /* 0001 */ CMD_RESET:
		case /* 0002 */ CMD_READ:
		case /* 0003 */ CMD_WRITE:
		case /* 0004 */ CMD_UPDATE:
		case /* 0005 */ CMD_CLEAR:
		// case /* 0006 */ CMD_STOP:
		// case /* 0007 */ CMD_START:
		// case /* 0008 */ CMD_FLUSH:
		case /* 0009 */ TD_MOTOR:
		case /* 000A */ TD_SEEK:
		case /* 000B */ TD_FORMAT:
		case /* 000D */ TD_CHANGENUM:
		case /* 000E */ TD_CHANGESTATE:
		case /* 000F */ TD_PROTSTATUS:
		case /* 0012 */ TD_GETDRIVETYPE:
		case /* 0016 */ TD_GETGEOMETRY:
		case /* 0017 */ TD_EJECT:
		case /* 0018 */ TD_READ64:
		case /* 0019 */ TD_WRITE64:
		case /* 001A */ TD_SEEK64:
		case /* 001B */ TD_FORMAT64:
		case /* 001C */ HD_SCSICMD:
		case /* 4000 */ NSCMD_DEVICEQUERY:
		case /* A004 */ NSCMD_TD_GETGEOMETRY64:			// Mounter.lib
		case /* A005 */	NSCMD_TD_CHANGEUNIT:			// Mounter.lib
		case /* C000 */ NSCMD_TD_READ64:
		case /* C001 */ NSCMD_TD_WRITE64:
		case /* C002 */ NSCMD_TD_SEEK64:
		case /* C003 */ NSCMD_TD_FORMAT64:
		{
			SEMAPHORE_OBTAIN( & MSDBase->msdbase_MSDDisk_Semaphore );

			queued = ( msddisk ) && ( ! msddisk->msddisk_MSDDev->msddev_Detached )
				&& ( msddisk->msddisk_MsgPort_Begin );

			if ( queued )
			{
				MSGPORT_PUTMSG( msddisk->msddisk_MsgPort_Begin, ioreq );
			}

			SEMAPHORE_RELEASE( & MSDBase->msdbase_MSDDisk_Semaphore );

			if ( ! queued )
			{
				if ( ioreq->io_Command == NSCMD_DEVICEQUERY )
				{
					MSD_Cmd_4000_NSCmd_DeviceQuery( usbbase, NULL, ioreq );
				}
				else if ( ( msddisk ) && ( ioreq->io_Command == TD_CHANGESTATE ) )
				{
					ioreq->io_Actual = 1;
					ioreq->io_Error = 0;
				}
				else
				{
					ioreq->io_Actual = 0;
					ioreq->io_Error = ( msddisk ) ? TDERR_DiskChanged : TDERR_BadUnitNum;
				}

				MSGPORT_REPLYMSG( ioreq );
			}
			break;
		}

		default:
		{
#ifdef MSD_DEBUG
			if ( msddisk )
			{
				MSDDEBUG( "MSD: BeginIO unit %lu unknown command $%04lX\n",
					msddisk->msddisk_UnitNr, (U32) ioreq->io_Command );
			}
			else
			{
				MSDDEBUG( "MSD: BeginIO unknown command $%04lX (no unit)\n", (U32) ioreq->io_Command );
			}
#else
			USBERROR( "MSD : MSD_Manager_BeginIO : Unknown Cmd ($%04lx)", (U32) ioreq->io_Command );
#endif

			ioreq->io_Error = IOERR_NOCMD;

			MSGPORT_REPLYMSG( ioreq );

			break;
		}
	}

	TASK_NAME_LEAVE();
}

// --
