
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static S32
Match_Active_Interfaces( const struct USB2_FktDriverNode *fdn, const struct USB2_ConfigNode *config )
{
	const struct USB2_Interface_Desc *desc;
	struct USB2_InterfaceGroup *group;
	struct USB2_InterfaceHeader *header;

	if ( ! config )
	{
		return ( FALSE );
	}

	for( group = config->cfg_InterfaceGroups.uh_Head; group; group = Node_Next( group ) )
	{
		for( header = group->ig_Group.uh_Head; header; header = Node_Next( header ) )
		{
			desc = header->ih_Active ? header->ih_Active->in_Descriptor : NULL;

			if ( ( desc ) && ( Match_Driver_Class( fdn, desc->InterfaceClass, desc->InterfaceSubClass, desc->InterfaceProtocol ) ) )
			{
				return ( TRUE );
			}
		}
	}

	return ( FALSE );
}

// --

SEC_CODE static const struct USB2_Interface_Desc *
Find_Active_Interface( const struct USB2_ConfigNode *config, USB2_ID id )
{
	struct USB2_InterfaceGroup *group;
	struct USB2_InterfaceHeader *header;

	if ( ! config )
	{
		return ( NULL );
	}

	for( group = config->cfg_InterfaceGroups.uh_Head; group; group = Node_Next( group ) )
	{
		for( header = group->ig_Group.uh_Head; header; header = Node_Next( header ) )
		{
			if ( ( header->ih_NotifyID == id ) && ( header->ih_Active ) )
			{
				return ( header->ih_Active->in_Descriptor );
			}
		}
	}

	return ( NULL );
}

// --

SEC_CODE static U32
Collect_Driver_Matches( struct USBBase *usbbase, struct RealFunctionNode *fn, const struct USB2_Interface_Desc *interface,
	U32 tag, STR buffer, U32 capacity )
{
	struct USB2_FktDriverNode *fdn;
	struct DriverSearch search;
	enum DriverDeviceMatch match;

	STR name;
	U32 length;
	U32 offset;
	S32 candidate;

	offset = 0;

	InitDriverSearch( usbbase, &search, NULL );

	for( fdn = Header_Head( &usbbase->usb_FDriver_Header ); fdn; fdn = Node_Next( fdn ) )
	{
		if ( ! TryDriverClass( &search, fdn ) )
		{
			continue;
		}

		match = Match_Driver_Device( usbbase, fdn, fn->fkt_DeviceDescriptor );

		if ( match == DriverMatch_None )
		{
			continue;
		}

		switch( fdn->fdn_Type )
		{
			case USB2Val_FDriver_Device:
				candidate = ( ( ! interface ) && ( fn->fkt_Tier ) && ( match == DriverMatch_ID ) );
				break;

			case USB2Val_FDriver_Function:
				candidate = ( ( ! interface ) && ( Match_Driver_Class( fdn, fn->fkt_Class, fn->fkt_SubClass, fn->fkt_Protocol ) ) );
				break;

			case USB2Val_FDriver_Interface:
				if ( interface )
				{
					candidate = Match_Driver_Class( fdn, interface->InterfaceClass, interface->InterfaceSubClass,
						interface->InterfaceProtocol );
				}
				else
				{
					candidate = ( ( tag != USB2Tag_Attr_DriverTitles ) && ( Match_Active_Interfaces( fdn, fn->fkt_Config_Active ) ) );
				}
				break;

			default:
				candidate = FALSE;
				break;
		}

		name = ( ( tag == USB2Tag_Attr_DriverMatches ) && ( fdn->fdn_Driver_Filename ) ) ?
			fdn->fdn_Driver_Filename : fdn->fdn_Title;

		if ( ( tag != USB2Tag_Attr_DriverMatches ) && ( ! name ) )
		{
			name = "Untitled driver";
		}

		if ( ( ! candidate ) || ( ! name ) || ( ! *name ) )
		{
			continue;
		}

		length = MISC_STRLENGTH( name );

		if ( offset )
		{
			if ( ( buffer ) && ( offset + 2 < capacity ) )
			{
				buffer[offset] = ',';
				buffer[offset + 1] = ' ';
			}

			offset += 2;
		}

		if ( ( buffer ) && ( offset + length < capacity ) )
		{
			MEM_COPY( name, &buffer[offset], length );
		}

		offset += length;
	}

	if ( ( buffer ) && ( capacity ) )
	{
		buffer[( offset < capacity ) ? offset : 0] = 0;
	}

	return ( offset + 1 );
}

// --

SEC_CODE U32
USB2_Driver_Matches( struct USBBase *usbbase, USB2_ID id, U32 tag, STR buffer, U32 capacity )
{
	struct RealFunctionNode *fn;
	struct USB2_Node *node;
	const struct USB2_Interface_Desc *interface;

	U32 length;

	length = 0;

	SEMAPHORE_OBTAIN( & usbbase->usb_FDriver_Semaphore );

	SEMAPHORE_OBTAIN( & usbbase->usb_Fkt_Semaphore );

	SEMAPHORE_OBTAIN( & usbbase->usb_LockSemaphore );

	for( node = usbbase->usb_Fkt_Header.uh_Tail; node; node = node->un_Prev )
	{
		fn = node->un_Data;

		if ( ( ! fn ) || ( ! fn->fkt_DeviceDescriptor ) || ( fn->fkt_Detach ) || ( fn->fkt_FreeMe ) )
		{
			continue;
		}

		interface = NULL;

		if ( fn->fkt_NotifyID != id )
		{
			if ( tag != USB2Tag_Attr_DriverTitles )
			{
				continue;
			}

			interface = Find_Active_Interface( fn->fkt_Config_Active, id );

			if ( ! interface )
			{
				continue;
			}
		}

		length = Collect_Driver_Matches( usbbase, fn, interface, tag, buffer, capacity );

		break;
	}

	SEMAPHORE_RELEASE( & usbbase->usb_LockSemaphore );

	SEMAPHORE_RELEASE( & usbbase->usb_Fkt_Semaphore );

	SEMAPHORE_RELEASE( & usbbase->usb_FDriver_Semaphore );

	return ( length );
}

// --
