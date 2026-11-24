
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static S32
Dot4_FindEndpoints( struct PrinterUnit *unit, struct USB2_DriverMessage *msg )
{
	const U8 *config;
	const U8 *desc;
	const U8 *end;

	U32 size;
	U8 match;

	config = (PTR)msg->ConfigDescriptors;
	match = FALSE;
	size = 0;

	if ( ( ! config ) || ( ! msg->Function ) ||
		( USB2_Attr_Get( msg->Function->FKT_ID, USB2Tag_Attr_ConfigSize, &size, sizeof( size ) ) != sizeof( size ) ) ||
		( size < 9 ) || ( config[0] < 9 ) || ( config[1] != DSCTYPE_Configuration ) ||
		( size != ( (U32)config[2] | ( (U32)config[3] << 8 ) ) ) )
	{
		return ( FALSE );
	}

	end = config + size;

	for ( desc = config ; desc < end ; desc += desc[0] )
	{
		if ( ( end - desc < 2 ) || ( desc[0] < 2 ) || ( desc[0] > end - desc ) )
		{
			return ( FALSE );
		}

		if ( desc[1] == DSCTYPE_Interface )
		{
			if ( match )
			{
				break;
			}

			match = ( ( desc[0] >= 9 ) && ( desc[2] == msg->InterfaceDescriptor->InterfaceNumber ) &&
				( desc[3] == msg->InterfaceDescriptor->AlternateSetting ) );
		}
		else if ( ( match ) && ( desc[1] == DSCTYPE_EndPoint ) && ( desc[0] >= 7 ) &&
			( ( desc[3] & 3 ) == EPATT_Type_Bulk ) && ( EPADR_Dir_Number( desc[2] ) ) )
		{
			if ( ( desc[2] & EPADR_Dir_Mask ) == EPADR_Dir_In )
			{
				if ( ! unit->unit_Dot4_InAddress )
				{
					unit->unit_Dot4_InAddress = desc[2];
				}
			}
			else if ( ! unit->unit_Dot4_OutAddress )
			{
				unit->unit_Dot4_OutAddress = desc[2];
			}
		}
	}

	return ( ( unit->unit_Dot4_InAddress ) && ( unit->unit_Dot4_OutAddress ) );
}

// --

S32
Process_Setup_Device( struct PrinterUnit *unit, struct USB2_DriverMessage *msg )
{
	struct USB2_IORequest *usbio;

	S32 retval;
	U32 cnt;

	retval = FALSE;

	USBERROR( "Printer : __myProcess_Setup_Device" );

	if ( ( unit->unit_Dot4_Active ) && ( ! Dot4_FindEndpoints( unit, msg ) ) )
	{
		goto bailout;
	}

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
		unit->unit_Dot4_Active ? USB2Tag_EPRes_EndPointNr : TAG_IGNORE, EPADR_Dir_Number( unit->unit_Dot4_OutAddress ),
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
			USB2Tag_EPRes_EndPointNr, EPADR_Dir_Number( unit->unit_Dot4_InAddress ),
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
