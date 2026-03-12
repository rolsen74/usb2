
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void _usb_Bulk_Stage_Send_Header1( struct USB_Struct *us, struct USB_Command *cmd )
{
struct USB2_IORequest *ioreq;
struct PTP_Container *con;

	MYINFO( "PTP-USB : _usb_Bulk_Stage_Send_Header1" );

	con = & cmd->ucmd_Container;
	con->TransID = LE_SWAP32( us->us_TransactionID );

	cmd->ucmd_Next = _usb_Bulk_Stage_Send_Header2;

	// Send
	ioreq = us->us_Res_BulkOut->IORequests[0];
	ioreq->io_Data = con;
	ioreq->io_Length = LE_SWAP32( con->Length );
	SendIO( (PTR) ioreq );
}

// --

void _usb_Bulk_Stage_Send_Header2( struct USB_Struct *us, struct USB_Command *cmd )
{
//struct USB2_IORequest *ioreq;
//
//	ioreq = us->us_Res_BulkOut->IORequests[0];

	MYINFO( "PTP-USB : _usb_Bulk_Stage_Send_Header2" );

	/**/ if ( cmd->ucmd_Type == CMDTYPE_Cmd_None_Stat )
	{
		_usb_Bulk_Stage_Read_Stat1( us, cmd );
	}
	else if ( cmd->ucmd_Type == CMDTYPE_Cmd_Read_Stat )
	{
		_usb_Bulk_Stage_Read_Data1( us, cmd );
	}
	else // ( cmd->ucmd_Type == CMDTYPE_Cmd_Send_Stat )
	{
		_usb_Bulk_Stage_Send_Data1( us, cmd );
	}
}

// --
