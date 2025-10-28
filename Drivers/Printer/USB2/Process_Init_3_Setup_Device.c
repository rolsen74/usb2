
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

S32 Process_Setup_Device( struct PrinterUnit *unit )
{
struct USB2_IORequest *usbio;
S32 retval;
U32 cnt;

	USBERROR( "Printer : __myProcess_Setup_Device" );

	retval = FALSE;

	unit->unit_USB_Register = USB2_Reg_RegisterTags(
		USB2Tag_Reg_Interface, 	unit->unit_USB_Interface,
//		USB2Tag_Reg_DriverMessage, in->StartMsg,
//		USB2Tag_Reg_TimeOut, 1000000*1,		// 1 sec
		USB2Tag_Reg_Title, "Printer",
		TAG_END
	);

	if ( ! unit->unit_USB_Register )
	{
		USBERROR( "Printer : Failed to Register Device : Ifc %p", unit->unit_USB_Interface );
		goto bailout;
	}

	unit->unit_Bulk_Tx_Resource = USB2_EPRes_ObtainTags( unit->unit_USB_Register,
		USB2Tag_EPRes_NrOfIORequest, NR_of_TXs,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_Out,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
//		USB2Tag_BufferSize, 1522,
		TAG_END
	);

	if ( ! unit->unit_Bulk_Tx_Resource )
	{
		USBERROR( "Printer : Error obtaining EndPoint Resource" );
		goto bailout;
	}

	for( cnt=0 ; cnt<NR_of_TXs ; cnt++ )
	{
		usbio = unit->unit_Bulk_Tx_Resource->IORequests[cnt];
		usbio->io_UserValue = cnt;

		AddTail( unit->unit_USB_IOReq_FreeList, (PTR) usbio );
	}

	retval = TRUE;

bailout:

	return( retval );
}
