
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "All.h"

// --

static void Send_FormFeed( struct PrinterUnit *unit )
{
struct USB2_IORequest *usbio;
U8 data[1];

	USBERROR( "Send_FormFeed" );

	// --
	// todo:
	//  Make sure Bulk queues are empty
	//  only run this when we have alost shutdown

	data[0] = '\f';		// FormFeed

	// high jack, first bulk... lets hope its done in this test
	usbio = unit->unit_Bulk_Tx_Resource->IORequests[0];

	usbio->io_Command	= CMD_WRITE;
	usbio->io_UserData	= NULL;
//	usbio->io_Offset	= 0;
	usbio->io_Length	= 1;
	usbio->io_Data		= data;
	DoIO( (PTR) usbio );

}

// --
