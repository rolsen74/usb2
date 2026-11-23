
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --
struct MsgPort *			USB2MsgPort				= NULL;
U32							USB2MsgPortBit			= 0;
struct IORequest *			USB2IOReq				= NULL;
struct Device *				USB2Base				= NULL;
struct USB2IFace *			IUSB2					= NULL;
// --
static struct MsgPort *		USBNotifyMsgPort		= NULL;
static PTR					USBHCDNotify			= NULL;
static PTR					USBFktNotify			= NULL;
static PTR					USBIfcNotify			= NULL;
U32							USBNotifyBit			= 0;
// --

static S32 Handle_Notify_HCD( struct USB2_NotifyMessage *msg );
static S32 Handle_Notify_Function( struct USB2_NotifyMessage *msg );
static S32 Handle_Notify_Interface( struct USB2_NotifyMessage *msg );

// --

S32 USB_Setup( void )
{
S32 retval;

	retval = FALSE;

	USB2MsgPort = AllocSysObjectTags( ASOT_PORT,
		TAG_END
	);

	if ( ! USB2MsgPort )
	{
		DebugPrintF( "Error Creating USB Device MsgPort\n" );
		goto bailout;
	}

	USB2MsgPortBit = 1UL << USB2MsgPort->mp_SigBit;

	USB2IOReq = (PTR)AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_ReplyPort,	USB2MsgPort,
		ASOIOR_Size,		sizeof( struct IORequest ),
		TAG_END
	);

	if ( ! USB2IOReq )
	{
		DebugPrintF( "Error Creating USB Device IORequest\n" );
		goto bailout;
	}

	if ( OpenDevice( "usb2.device", 0, USB2IOReq, 0 ))
	{
		DebugPrintF( "Error Opening usb2.device\n" );
		goto bailout;
	}

	USB2Base = (PTR) USB2IOReq->io_Device;
	IUSB2 = (PTR) GetInterface( (PTR) USB2Base, "main", 1, NULL );

	if ( ! IUSB2 )
	{
		DebugPrintF( "Error Obtaining USB Interface\n" );
		goto bailout;
	}

	USBNotifyMsgPort = AllocSysObjectTags( ASOT_PORT,
		TAG_END
	);

	if ( ! USBNotifyMsgPort )
	{
		DebugPrintF( "Error allocating MsgPort" );
		goto bailout;
	}

	USBNotifyBit = 1UL << USBNotifyMsgPort->mp_SigBit;

	USBHCDNotify = USB2_Notify_Add( USBNT_HCD, USBNotifyMsgPort );

	if ( ! USBHCDNotify )
	{
		DebugPrintF( "Error setting up USB Notify" );
		goto bailout;
	}

	USBFktNotify = USB2_Notify_Add( USBNT_Function, USBNotifyMsgPort );

	if ( ! USBFktNotify )
	{
		DebugPrintF( "Error setting up USB Notify" );
		goto bailout;
	}

	USBIfcNotify = USB2_Notify_Add( USBNT_Interface, USBNotifyMsgPort );

	if ( ! USBIfcNotify )
	{
		DebugPrintF( "Error setting up USB Notify" );
		goto bailout;
	}

	retval = TRUE;

bailout:

	return( retval );
}

// --

void USB_Cleanup( void )
{
PTR msg;

	if ( USBIfcNotify )
	{
		USB2_Notify_Remove( USBIfcNotify );
		USBIfcNotify = NULL;
	}

	if ( USBFktNotify )
	{
		USB2_Notify_Remove( USBFktNotify );
		USBFktNotify = NULL;
	}

	if ( USBHCDNotify )
	{
		USB2_Notify_Remove( USBHCDNotify );
		USBHCDNotify = NULL;
	}

	if ( USBNotifyMsgPort )
	{
		while(( msg = GetMsg( USBNotifyMsgPort )))
		{
			ReplyMsg( msg );
		}

		FreeSysObject( ASOT_PORT, USBNotifyMsgPort );
		USBNotifyMsgPort = NULL;
	}

	if ( IUSB2 )
	{
		DropInterface( (PTR) IUSB2 );
		IUSB2 = NULL;
	}

	if ( USB2Base )
	{
		CloseDevice( USB2IOReq );
		USB2Base = NULL;
	}

	if ( USB2IOReq )
	{
		FreeSysObject( ASOT_IOREQUEST, USB2IOReq );
		USB2IOReq = NULL;
	}

	if ( USB2MsgPort )
	{
		FreeSysObject( ASOT_PORT, USB2MsgPort );
		USB2MsgPort = NULL;
	}
}

// --

void Handle_Notify( void )
{
struct USB2_NotifyMessage *msg;
S32 rebuild;

	rebuild = FALSE;

	while(( msg = (PTR) GetMsg( USBNotifyMsgPort )))
	{
		switch( msg->nm_Type )
		{
			case USBNT_HCD:
			{
				rebuild |= Handle_Notify_HCD( msg );
				break;
			}

			case USBNT_Function:
			{
				rebuild |= Handle_Notify_Function( msg );
				break;
			}

			case USBNT_Interface:
			{
				rebuild |= Handle_Notify_Interface( msg );
				break;
			}

			default:
			{
				DebugPrintF( "Unknown Notify Type #%u\n", msg->nm_Type );
				break;
			}
		}

		ReplyMsg( & msg->nm_Message );
	}

	if ( ( rebuild ) && ( Main_WinObject ) )
	{
		Build_Device_Tree();
	}
}

// --

static STR HCDNames[HCDTYPE_Last] =
{
	[HCDTYPE_UHCI]		= "UHCI",	// PCI
	[HCDTYPE_OHCI]		= "OHCI",	// PCI
	[HCDTYPE_EHCI]		= "EHCI",	// PCI
	[HCDTYPE_XHCI]		= "XHCI",	// PCI
	[HCDTYPE_TABOR]		= "EHCI",	// Tabor Onboard EHCI controller (A1222)
	[HCDTYPE_P5020]		= "EHCI",	// X5020 Onboard EHCI Controller
	[HCDTYPE_P5040]		= "EHCI",	// X5040 Onboard EHCI Controller
	[HCDTYPE_SAM440]	= "OHCI",	// SAM440 Onboard OHCI controller
	[HCDTYPE_SAM460]	= "OHCI",	// SAM460 Onboard OHCI controller
};

static S32 Handle_Notify_HCD( struct USB2_NotifyMessage *msg )
{
struct USB2_FktNode *node;
STR name;
S32 rebuild;

	rebuild = FALSE;

	switch( msg->nm_Command )
	{
		case USBNC_Adding:
		{
			node = FktNode_Alloc( TYPE_HCD, msg->nm_ID );

			if ( ! node )
			{
				break;
			}

			if ( msg->nm_Data.nm_HCD.nm_Type < HCDTYPE_Last )
			{
				name = HCDNames[msg->nm_Data.nm_HCD.nm_Type];
			}
			else
			{
				name = NULL;
			}

			if ( name )
			{
				name = ASPrintf( "%s #%ld", name, msg->nm_Data.nm_HCD.nm_Index );
			}
			else
			{
				name = ASPrintf( "Unknown #%ld", msg->nm_Data.nm_HCD.nm_Index );
			}

			node->Name = name;

			// --

			AddTail( & DeviceTreeUSBList, (PTR) node );

			rebuild = TRUE;
			break;
		}

		case USBNC_Updating:
		{
			node = FktNode_Find( & DeviceTreeUSBList, msg->nm_ID );

			if ( node )
			{

				// There are nothing that can change atm

			}
			break;
		}

		case USBNC_Removing:
		{
			node = FktNode_Find( & DeviceTreeUSBList, msg->nm_ID );

			if ( node )
			{
				Remove( (PTR) node );

				FktNode_Delete( node );

				rebuild = TRUE;
			}
			break;
		}

		default:
		{
			DebugPrintF( "HCD : Unknown : %lx\n", msg->nm_ID );
			break;
		}
	}

	return(	rebuild );
}

// --

static void
Read_Device_String( USB2_ID id, U32 tag, STR text, U32 size )
{
	U8 descriptor[255];
	U32 actual;
	U32 length;
	U32 index;
	U32 used;
	U16 code;

	text[0] = 0;
	used = 0;

	actual = USB2_Attr_Get( id, tag, descriptor, sizeof( descriptor ) );

	if ( actual < 4 )
	{
		return;
	}

	length = descriptor[0];

	if ( ( length < 4 ) || ( length > actual ) || ( length & 1 ) || ( descriptor[1] != DSCTYPE_String ) )
	{
		return;
	}

	for( index = 2; ( index < length ) && ( used + 1 < size ); index += 2 )
	{
		code = descriptor[index] | ( (U16) descriptor[index + 1] << 8 );

		if ( ! code )
		{
			break;
		}

		text[used++] = ( ( code >= 32 && code < 127 ) || ( code >= 160 && code <= 255 ) ) ? code : '?';
	}

	text[used] = 0;
}

// --

static S32 Handle_Notify_Function( struct USB2_NotifyMessage *msg )
{
struct USB2_HUB_Desc HUBDsc;
struct USB2_FktNode *parent;
struct USB2_FktNode *node;
struct USB2_FktNode *child;
U32 ports;
S32 rebuild;

	rebuild = FALSE;

	parent = FktNode_Find( & DeviceTreeUSBList, msg->nm_Data.nm_Function.nm_Parent_ID );

	if ( ! parent )
	{
		DebugPrintF( "Parent Node not found\n" );
		goto bailout;
	}

	switch( msg->nm_Command )
	{
		case USBNC_Adding:
		{
			if ( FktNode_Find( & DeviceTreeUSBList, msg->nm_ID ) )
			{
				break;
			}

			node = FktNode_Alloc( TYPE_FUNCTION, msg->nm_ID );

			if ( ! node )
			{
				DebugPrintF( "Error Allocating Memory" );
				break;
			}

			node->Port	= msg->nm_Data.nm_Function.nm_PortNr;

			// --

			if ( USB2_Attr_Get( msg->nm_ID, USB2Tag_Attr_DeviceDesc,
				& node->DevDsc, sizeof( node->DevDsc ) ) != sizeof( node->DevDsc ) )
			{
				FktNode_Delete( node );
				break;
			}

			// --

			if ( node->DevDsc.DeviceClass == USBCLASS_HUB )
			{
				ports = 0;

				if ( USB2_Attr_Get( msg->nm_ID, USB2Tag_Attr_HUBPorts,
					& ports, sizeof( ports ) ) != sizeof( ports ) )
				{
					if ( USB2_Attr_Get( msg->nm_ID, USB2Tag_Attr_HUBDesc,
						& HUBDsc, sizeof( HUBDsc ) ) == sizeof( HUBDsc ) )
					{
						ports = HUBDsc.NbrPorts;
					}
				}

				node->Ports = ports;

				if ( node->Ports )
				{
					node->Functions = AllocVecTags( node->Ports * sizeof( *node->Functions ),
						AVT_Clear, 0,
						TAG_END
					);

					if ( ! node->Functions )
					{
						DebugPrintF( "Error Allocating Memory" );
						FktNode_Delete( node );
						break;
					}
				}
			}

			if ( ( parent->Functions ) &&
				( ( node->Port < 1 ) || ( node->Port > parent->Ports ) ) )
			{
				FktNode_Delete( node );
				break;
			}

			// --

			if ( ( USB2_Attr_Get( msg->nm_ID, USB2Tag_Attr_ConfigSize,
				& node->CfgSize, sizeof( U32 ) ) != sizeof( U32 ) ) ||
				( node->CfgSize <= 0 ) || ( node->CfgSize > 65535 ) )
			{
				FktNode_Delete( node );
				break;
			}
			// IUSB2->Stack_GetAttribute( msg->nm_ID, USB2Tag_FKT_ConfigDescriptorSize, &node->Data.Function.ConfigSize, sizeof( U32 ));

			node->CfgDesc = AllocVecTags( node->CfgSize + 2,	// Adding empty Type+Len
				AVT_Clear, 0,
				TAG_END
			);

			if ( ! node->CfgDesc )
			{
				FktNode_Delete( node );
				break;
			}

			if ( USB2_Attr_Get( msg->nm_ID, USB2Tag_Attr_ConfigDesc,
				node->CfgDesc, (U32) node->CfgSize ) != (U32) node->CfgSize )
			{
				FktNode_Delete( node );
				break;
			}
			// IUSB2->Stack_GetAttribute( msg->nm_ID, USB2Tag_FKT_ConfigDescriptor, node->Data.Function.ConfigDesc, node->Data.Function.ConfigSize );

			// --

			node->Speed = ~0U;

			USB2_Attr_Get( node->ID, USB2Tag_Attr_Speed, & node->Speed, sizeof( node->Speed ) );

			Read_Device_String( node->ID, USB2Tag_Attr_ManufacturerStr, node->ManufacturerStr, sizeof( node->ManufacturerStr ) );

			Read_Device_String( node->ID, USB2Tag_Attr_ProductStr, node->ProductStr, sizeof( node->ProductStr ) );

			Read_Device_String( node->ID, USB2Tag_Attr_SerialStr, node->SerialStr, sizeof( node->SerialStr ) );

			// --

			if ( parent->Functions )
			{
				parent->Functions[ node->Port-1 ] = node;
			}

			AddTail( & parent->Header, (PTR) node );

			rebuild = TRUE;
			break;
		}

		case USBNC_Updating:
		{
			node = FktNode_Find( & DeviceTreeUSBList, msg->nm_ID );

			if ( node )
			{
				if ( node->DevDsc.DeviceClass == USBCLASS_HUB )
				{
					ports = 0;

					if ( USB2_Attr_Get( msg->nm_ID, USB2Tag_Attr_HUBPorts,
						& ports, sizeof( ports ) ) != sizeof( ports ) )
					{
						if ( USB2_Attr_Get( msg->nm_ID, USB2Tag_Attr_HUBDesc,
							& HUBDsc, sizeof( HUBDsc ) ) == sizeof( HUBDsc ) )
						{
							ports = HUBDsc.NbrPorts;
						}
					}

					if ( ( node->Ports == 0 ) && ( ports > 0 ) )
					{
						node->Functions = AllocVecTags( ports * sizeof( *node->Functions ),
							AVT_Clear, 0,
							TAG_END
						);

						if ( node->Functions )
						{
							node->Ports = ports;

							for ( child = (PTR) GetHead( & node->Header ); child;
								child = (PTR) GetSucc( (PTR) child ) )
							{
								if ( ( child->Type == TYPE_FUNCTION ) &&
									( child->Port >= 1 ) && ( child->Port <= node->Ports ) )
								{
									node->Functions[child->Port-1] = child;
								}
							}
						}
					}

					// The descriptor can become available with the port count unchanged.
					rebuild = TRUE;
				}
			}
			break;
		}

		case USBNC_Removing:
		{
			node = FktNode_Find( & DeviceTreeUSBList, msg->nm_ID );

			if ( ( node ) && ( parent->Functions ) &&
				( node->Port >= 1 ) && ( node->Port <= parent->Ports ) &&
				( parent->Functions[node->Port-1] == node ) )
			{
				parent->Functions[ node->Port-1 ] = NULL;
			}

			if ( node )
			{
				Remove( (PTR) node );
				FktNode_Delete( node );

				rebuild = TRUE;
			}
			break;
		}

		default:
		{
			break;
		}
	}

bailout:

	return(	rebuild );
}

// --

static S32
Select_Interface_Descriptor( struct USB2_FktNode *node, struct USB2_FktNode *parent,
	const struct USB2_NotifyMessage *msg )
{
	struct USB2_Descriptor *dsc;
	struct USB2_Config_Desc *config;
	struct USB2_Interface_Desc *interface;

	U32 number;
	S32 length;

	dsc = (PTR) parent->CfgDesc;
	length = parent->CfgSize;
	number = ~0U;

	while ( ( dsc ) && ( length >= 2 ) )
	{
		if ( ( dsc->Length < 2 ) || ( dsc->Length > length ) )
		{
			break;
		}

		if ( ( dsc->DescriptorType == DSCTYPE_Configuration ) &&
			( dsc->Length >= sizeof( *config ) ) )
		{
			config = (PTR) dsc;
			number = config->ConfigurationValue;
		}
		else if ( ( dsc->DescriptorType == DSCTYPE_Interface ) &&
			( dsc->Length >= sizeof( *interface ) ) &&
			( number == msg->nm_Data.nm_Interface.nm_Config_Nr ) )
		{
			interface = (PTR) dsc;

			if ( ( interface->InterfaceNumber == msg->nm_Data.nm_Interface.nm_Interface_Nr ) &&
				( interface->AlternateSetting == msg->nm_Data.nm_Interface.nm_Interface_AltNr ) )
			{
				node->IfcDesc = interface;
				node->IfcSize = length;
				node->Config = number;
				node->Speed = parent->Speed;

				return ( TRUE );
			}
		}

		length -= dsc->Length;
		dsc = (PTR) ( (U8 *) dsc + dsc->Length );
	}

	return ( FALSE );
}

// --

static S32
Handle_Notify_Interface( struct USB2_NotifyMessage *msg )
{
	struct USB2_FktNode *parent;
	struct USB2_FktNode *node;

	node = FktNode_Find( & DeviceTreeUSBList, msg->nm_ID );

	if ( msg->nm_Command == USBNC_Removing )
	{
		if ( ! node )
		{
			return ( FALSE );
		}

		Remove( (PTR) node );

		FktNode_Delete( node );

		return ( TRUE );
	}

	if ( ( msg->nm_Command != USBNC_Adding ) && ( msg->nm_Command != USBNC_Updating ) )
	{
		return ( FALSE );
	}

	parent = FktNode_Find( & DeviceTreeUSBList, msg->nm_Data.nm_Interface.nm_Function_ID );

	if ( ! parent )
	{
		return ( FALSE );
	}

	if ( node )
	{
		return ( Select_Interface_Descriptor( node, parent, msg ) );
	}

	node = FktNode_Alloc( TYPE_INTERFACE, msg->nm_ID );

	if ( ! node )
	{
		return ( FALSE );
	}

	if ( ! Select_Interface_Descriptor( node, parent, msg ) )
	{
		FktNode_Delete( node );

		return ( FALSE );
	}

	AddTail( & parent->Header, (PTR) node );

	return ( TRUE );
}

// --

STR GetClassName( U32 class, U32 subclass, U32 protocol )
{
STR name;

	switch( class )
	{
		case USBCLASS_Audio:
		{
			switch( subclass )
			{
				case 0x01:
				{
					name = "Audio Control";
					break;
				}

				case 0x02:
				{
					name = "Audio Streaming";
					break;
				}

				case 0x03:
				{
					name = "Audio MIDI Streaming";
					break;
				}

				default:
				{
					name = "Audio";
					break;
				}
			}
			break;
		}

		case USBCLASS_CDC_Control:
		{
			name = "Communications and CDC Control";
			break;
		}

		case USBCLASS_HID:
		{
			name = "HID Device";

			if ( subclass == 1 ) // Boot Protocol mode
			{
				switch( protocol )
				{
					case 1:
					{
						name = "HID Keyboard";
						break;
					}

					case 2:
					{
						name = "HID Mouse";
						break;
					}

					default:
					{
						break;
					}
				}
			}

			break;
		}

		case USBCLASS_Physical:
		{
			name = "Physical";
			break;
		}

		case USBCLASS_Image:
		{
			name = "Image";
			break;
		}

		case USBCLASS_Printer:
		{
			switch( protocol )
			{
				case 0x01:
				{
					name = "Unidirectional Printer";
					break;
				}

				case 0x02:
				{
					name = "Bidirectional Printer";
					break;
				}

				case 0x03:
				{
					name = "IEEE1284.4 Printer (Dot4)";
					break;
				}

				case 0x04:
				{
					name = "Vendor Specific Printer";
					break;
				}

				default:
				{
					name = "Printer";
					break;
				}
			}
			break;
		}

		case USBCLASS_Mass_Storage:
		{
			switch( protocol )
			{
				default:
				{
					name = "Mass Storage";
					break;
				}
			}
			break;
		}

		case USBCLASS_HUB:
		{
			name = "Hub";
			break;
		}

		case USBCLASS_CDC_Data:
		{
			name = "CDC Data";
			break;
		}

		case USBCLASS_Smart_Card:
		{
			name = "Smart Card";
			break;
		}

		case USBCLASS_Content_Security:
		{
			name = "Content Security";
			break;
		}

		case USBCLASS_Video:
		{
			switch( subclass )
			{
				case 0x01:
				{
					name = "Video Control";
					break;
				}

				case 0x02:
				{
					name = "Video Streaming";
					break;
				}

				case 0x03:
				{
					name = "Video Collection";
					break;
				}

				default:
				{
					name = "Video";
					break;
				}
			}
			break;
		}

		case USBCLASS_Personal_Healthcase:
		{
			name = "Personal Healthcare";
			break;
		}

		case USBCLASS_Diagnostic_Device:
		{
			name = "Diagnostic Device";
			break;
		}

		case USBCLASS_Wireless_Controller:
		{
			name = "Wireless Controller";
			break;
		}

		case USBCLASS_Miscellaneous:
		{
			name = "Miscellaneous";
			break;
		}

		case USBCLASS_Application_Specific:
		{
			name = "Application";
			break;
		}

		default:
		case USBCLASS_Vendor_Specific:
		{
			name = "Vendor Specific";
			break;
		}
	}

	return( name );
}

// --
