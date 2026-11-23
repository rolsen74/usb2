
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
Process_Setup_Device( struct PrinterUnit *unit, struct USB2_DriverMessage *msg )
{
	struct USB2_IORequest *usbio;

	S32 retval;
	U32 cnt;

	retval = FALSE;

	USBERROR( "Printer : __myProcess_Setup_Device" );

	unit->unit_USB_Register = USB2_Reg_RegisterTags(
		USB2Tag_Reg_DriverMessage, msg,
		USB2Tag_Reg_Title, "Printer",
		TAG_END
	);

	if ( ! unit->unit_USB_Register )
	{
		USBERROR( "Printer : Failed to Register Device : Ifc %p", unit->unit_USB_Interface );

		goto bailout;
	}

	unit->unit_Bulk_Tx_Resource = USB2_EPRes_ObtainTags( unit->unit_USB_Register,
		USB2Tag_EPRes_InterfaceNr, unit->unit_USB_Interface->IFC_Num,
		USB2Tag_EPRes_NrOfIORequest, NR_of_TXs,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_Out,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_TimeOut, PRINTER_TX_TIMEOUT_US,
//		USB2Tag_BufferSize, 1522,
		TAG_END
	);

	if ( ! unit->unit_Bulk_Tx_Resource )
	{
		USBERROR( "Printer : Error obtaining EndPoint Resource" );

		goto bailout;
	}

	if ( unit->unit_Dot4_Active )
	{
		unit->unit_Bulk_Rx_Resource = USB2_EPRes_ObtainTags( unit->unit_USB_Register,
			USB2Tag_EPRes_InterfaceNr, unit->unit_USB_Interface->IFC_Num,
			USB2Tag_EPRes_NrOfIORequest, 1,
			USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
			USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
			USB2Tag_EPRes_TimeOut, 2000000,
			TAG_END
		);

		if (( ! unit->unit_Bulk_Rx_Resource ) || ( ! Dot4_Setup( unit )))
		{
			USBERROR( "Printer : DOT4 channel setup failed" );

			goto bailout;
		}

	}

	for( cnt=0 ; cnt<NR_of_TXs ; cnt++ )
	{
		usbio = unit->unit_Bulk_Tx_Resource->IORequests[cnt];
		usbio->io_UserValue = cnt;

		if ( ! unit->unit_Dot4_Active )
		{
			AddTail( unit->unit_USB_IOReq_FreeList, (PTR) usbio );
		}
	}

	retval = TRUE;

bailout:

	return ( retval );
}
