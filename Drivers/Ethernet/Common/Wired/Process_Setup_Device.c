
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

int
unit_Process_InitUSB( struct devUnit *unit, struct USB2_DriverMessage *msg )
{
	struct USB2IFace *IUSB2;
	struct IONode *node;

	U32 index;

	IUSB2 = unit->unit_IUSB2;

	unit->unit_USBRegister = IUSB2->USB2_Reg_RegisterTags(
		USB2Tag_Reg_DriverMessage, msg,
		USB2Tag_Reg_Title, WIRED_NAME " Ethernet",
		TAG_END );

	if ( ( ! unit->unit_USBRegister ) || ( ! unit->unit_USBRegister->Res_Control ) ||
		( ! unit->unit_USBRegister->Res_Control->IORequests ) ||
		( ! unit->unit_USBRegister->Res_Control->SetupData ) )
	{
		return ( FALSE );
	}

	unit->unit_USBControlIOReq = unit->unit_USBRegister->Res_Control->IORequests[0];

	if ( ! unit->unit_USBControlIOReq )
	{
		return ( FALSE );
	}

	unit->unit_USBBulkIn = IUSB2->USB2_EPRes_ObtainTags( unit->unit_USBRegister,
		USB2Tag_EPRes_InterfaceNr, unit->unit_USBInterface->IFC_Num,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_In,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_NrOfIORequest, WIRED_RX_REQUEST_COUNT,
		USB2Tag_EPRes_BufferSize, WIRED_RX_BUFFER_SIZE,
		USB2Tag_EPRes_AllowShortPackets, TRUE,
		TAG_END );

	if ( ! unit->unit_USBBulkIn )
	{
		return ( FALSE );
	}

	unit->unit_USBBulkOut = IUSB2->USB2_EPRes_ObtainTags( unit->unit_USBRegister,
		USB2Tag_EPRes_InterfaceNr, unit->unit_USBInterface->IFC_Num,
		USB2Tag_EPRes_EPDirection, EPADR_Dir_Out,
		USB2Tag_EPRes_EPType, EPATT_Type_Bulk,
		USB2Tag_EPRes_NrOfIORequest, WIRED_TX_REQUEST_COUNT,
		USB2Tag_EPRes_BufferSize, WIRED_TX_BUFFER_SIZE,
		TAG_END );

	if ( ! unit->unit_USBBulkOut )
	{
		return ( FALSE );
	}

	for( index = 0; index < WIRED_RX_REQUEST_COUNT; index++ )
	{
		node = AllocIONode( unit->unit_USBBulkIn, index );

		if ( ! node )
		{
			return ( FALSE );
		}

		IExec->AddTail( & unit->unit_USBBulkInFreeList, & node->Node );
	}

	for( index = 0; index < WIRED_TX_REQUEST_COUNT; index++ )
	{
		node = AllocIONode( unit->unit_USBBulkOut, index );

		if ( ! node )
		{
			return ( FALSE );
		}

		IExec->AddTail( & unit->unit_USBBulkOutFreeList, & node->Node );
	}

	return ( TRUE );
}

// --
