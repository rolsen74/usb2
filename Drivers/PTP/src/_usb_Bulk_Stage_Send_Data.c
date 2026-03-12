
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

void _usb_Bulk_Stage_Send_Data1( struct USB_Struct *us, struct USB_Command *cmd )
{
	MYINFO( "PTP-USB : _usb_Bulk_Stage_Send_Data1 : TransID %ld", us->us_TransactionID );
	MYINFO( "PTP-USB : _usb_Bulk_Stage_Read_Data1 : MaxP %ld", us->us_Res_BulkOut->EndPointMax );

}

// --
