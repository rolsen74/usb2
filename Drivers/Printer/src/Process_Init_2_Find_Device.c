
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
Process_Find_Device( struct PrinterUnit *unit )
{
	struct USB2_Interface *ifc;

	U8 *config;
	U32 configsize;
	U32 offset;
	S32 retval;

	retval = FALSE;

	unit->unit_Dot4_Active = FALSE;
	unit->unit_Dot4_Ready = FALSE;
	unit->unit_Dot4_Socket = 0;
	unit->unit_Dot4_Credits = 0;
	unit->unit_Dot4_PacketSize = 0;
	unit->unit_Dot4_RxUsed = 0;

	USBERROR( "Printer : Process_Find_Device" );

	// USB ID is 16bit, so anything over 65535 is a unset value
	if (( unit->unit_USB_VendorID < 65536 )
	&&	( unit->unit_USB_DeviceID < 65536 ))
	{
		USBERROR( "Printer : __Find_DeviceID : Find DevID $%04lx:%04lx", unit->unit_USB_VendorID, unit->unit_USB_DeviceID );

		ifc = USB2_Ifc_FindTags(
			USB2Tag_Find_VendorID, unit->unit_USB_VendorID,
			USB2Tag_Find_DeviceID, unit->unit_USB_DeviceID,
	//		USB2Tag_Find_SeeClaimed, TRUE,
	//		USB2Tag_Find_Index
			TAG_END
		);
	}
	else
	{
		USBERROR( "Printer : __Find_Generic" );

		ifc = USB2_Ifc_FindTags(
			USB2Tag_Find_SubClass, USBPRT_SUBCLASS_Printer,
			USB2Tag_Find_Class, USBCLASS_Printer,
			USB2Tag_Find_SeeClaimed, TRUE,
	//		USB2Tag_Find_Index
			TAG_END
		);
	}

	if ( ! ifc )
	{
		USBERROR( "Printer : Failed to find Printer" );

		goto bailout;
	}

	unit->unit_USB_Interface = ifc;

	// Match the selected interface with its descriptor. Protocol 3 uses
	// IEEE 1284.4 packets and must never receive a raw printer stream.
	configsize = 0;

	if ( USB2_Attr_Get( ifc->FKT_ID, USB2Tag_Attr_ConfigSize, &configsize, sizeof( configsize )) != sizeof( configsize ))
	{
		goto bailout;
	}

	if (( configsize < sizeof( struct USB2_Config_Desc )) || ( configsize > 65535 ))
	{
		goto bailout;
	}

	config = AllocVecTags( configsize, TAG_END );

	if ( ! config )
	{
		goto bailout;
	}

	if ( USB2_Attr_Get( ifc->FKT_ID, USB2Tag_Attr_ConfigDesc, config, configsize ) != configsize )
	{
		FreeVec( config );

		goto bailout;
	}

	for ( offset = 0; offset + 2 <= configsize; offset += config[offset] )
	{
		struct USB2_Interface_Desc *desc;

		if (( config[offset] < 2 ) || ( offset + config[offset] > configsize ))
		{
			break;
		}

		if (( config[offset+1] != 4 ) || ( config[offset] < sizeof( *desc )))
		{
			continue;
		}

		desc = (PTR) &config[offset];

		if (( desc->InterfaceNumber != ifc->IFC_Num ) || ( desc->AlternateSetting != 0 ))
		{
			continue;
		}

		if (( desc->InterfaceClass != USBCLASS_Printer ) || ( desc->InterfaceSubClass != USBPRT_SUBCLASS_Printer ))
		{
			break;
		}

		if ( desc->InterfaceProtocol == USBPRT_PROTOCOL_IEEE1284_4 )
		{
			unit->unit_Dot4_Active = TRUE;
		}
		else if (( desc->InterfaceProtocol != USBPRT_PROTOCOL_UNIDIRECTIONAL )
		&& ( desc->InterfaceProtocol != USBPRT_PROTOCOL_BIDIRECTIONAL ))
		{
			break;
		}

		retval = TRUE;
		break;
	}

	FreeVec( config );


bailout:

	return ( retval );
}

// --
