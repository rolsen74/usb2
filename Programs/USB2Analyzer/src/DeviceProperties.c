
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static void
Add_Property( struct List *list, const char *property, const char *format, ... )
{
	struct Node *row;

	va_list args;
	char value[256];

	va_start( args, format );

	vsnprintf( value, sizeof( value ), format, args );

	va_end( args );

	row = AllocListBrowserNode( 2,
		LBNA_Column, 0,
		 LBNCA_CopyText, TRUE,
		 LBNCA_Text, property,
		LBNA_Column, 1,
		 LBNCA_CopyText, TRUE,
		 LBNCA_Text, value,
		TAG_END
	);

	if ( row )
	{
		AddTail( list, row );
	}
}

// --

static void
Add_Class( struct List *list, U8 class, U8 subclass, U8 protocol )
{
	const char *name;

	name = class ? GetClassName( class, subclass, protocol ) : "Defined by interfaces";

	Add_Property( list, "Class / Subclass / Protocol", "%02X / %02X / %02X (%s)",
		(unsigned) class, (unsigned) subclass, (unsigned) protocol, name );
}

// --

static void
Add_Interface( struct List *list, const struct USB2_Interface_Desc *interface )
{
	char property[64];

	snprintf( property, sizeof( property ), "Interface %u, alternate %u",
		(unsigned) interface->InterfaceNumber, (unsigned) interface->AlternateSetting );

	Add_Property( list, property, "%u bytes, %u endpoints, string index %u",
		(unsigned) interface->Length, (unsigned) interface->NumEndPoints, (unsigned) interface->InterfaceStrNr );

	Add_Class( list, interface->InterfaceClass, interface->InterfaceSubClass, interface->InterfaceProtocol );
}

// --

static void
Add_Endpoint( struct List *list, const struct USB2_EndPoint_Desc *endpoint )
{
	static const char * const types[] = { "Control", "Isochronous", "Bulk", "Interrupt" };

	char property[32];
	U16 packet;

	packet = LE_SWAP16( endpoint->MaxPacketSize );

	snprintf( property, sizeof( property ), "  Endpoint 0x%02X", (unsigned) endpoint->EndPointAddress );

	Add_Property( list, property, "%s %s, %u bytes/packet, interval %u",
		types[endpoint->Attributes & EPATT_Type_Mask],
		( endpoint->EndPointAddress & EPADR_Dir_Mask ) ? "IN" : "OUT",
		(unsigned) ( packet & 0x07ff ), (unsigned) endpoint->Interval );

	Add_Property( list, "    Endpoint descriptor", "%u bytes, attributes 0x%02X, wMaxPacketSize 0x%04X",
		(unsigned) endpoint->Length, (unsigned) endpoint->Attributes, (unsigned) packet );
}

// --

static void
Add_Descriptor_Bytes( struct List *list, const struct USB2_Descriptor *descriptor, U32 offset )
{
	static const char hex[] = "0123456789ABCDEF";

	const U8 *data;
	U32 start;
	U32 index;
	U32 used;
	char property[24];
	char value[3 * 16];

	data = (const U8 *) descriptor;

	for( start = offset; start < descriptor->Length; start = index )
	{
		used = 0;

		for( index = start; ( index < descriptor->Length ) && ( index - start < 16 ); index++ )
		{
			if ( used )
			{
				value[used++] = ' ';
			}

			value[used++] = hex[data[index] >> 4];
			value[used++] = hex[data[index] & 15];
		}

		value[used] = 0;

		snprintf( property, sizeof( property ), "    Bytes +0x%02X", (unsigned) start );

		Add_Property( list, property, "%s", value );
	}
}

// --

static void
Add_Descriptor_List( struct List *list, const U8 *data, U32 length, S32 interface_only )
{
	const struct USB2_Descriptor *descriptor;
	const struct USB2_Association_Desc *association;

	U32 offset;
	U32 decoded;

	offset = 0;

	while( length - offset >= sizeof( *descriptor ) )
	{
		descriptor = (const struct USB2_Descriptor *) ( data + offset );

		if ( ( descriptor->Length < sizeof( *descriptor ) ) || ( descriptor->Length > length - offset ) )
		{
			break;
		}

		if ( ( descriptor->DescriptorType == DSCTYPE_Configuration ) ||
			( ( interface_only ) && ( ( descriptor->DescriptorType == DSCTYPE_Interface ) ||
				( descriptor->DescriptorType == DSCTYPE_Interface_Association ) ) ) )
		{
			return;
		}

		decoded = 0;

		if ( ( descriptor->DescriptorType == DSCTYPE_Interface ) && ( descriptor->Length >= sizeof( struct USB2_Interface_Desc ) ) )
		{
			Add_Interface( list, (const struct USB2_Interface_Desc *) descriptor );

			decoded = sizeof( struct USB2_Interface_Desc );
		}
		else if ( ( descriptor->DescriptorType == DSCTYPE_EndPoint ) && ( descriptor->Length >= sizeof( struct USB2_EndPoint_Desc ) ) )
		{
			Add_Endpoint( list, (const struct USB2_EndPoint_Desc *) descriptor );

			decoded = sizeof( struct USB2_EndPoint_Desc );
		}
		else if ( ( descriptor->DescriptorType == DSCTYPE_Interface_Association ) &&
			( descriptor->Length >= sizeof( *association ) ) )
		{
			association = (const struct USB2_Association_Desc *) descriptor;

			Add_Property( list, "Interface association", "%u bytes, first interface %u, %u interfaces, string index %u",
				(unsigned) association->Length, (unsigned) association->FirstInterface,
				(unsigned) association->InterfaceCount, (unsigned) association->Function );

			Add_Class( list, association->FunctionClass, association->FunctionSubClass, association->FunctionProtocol );

			decoded = sizeof( *association );
		}
		else
		{
			Add_Property( list, "  Descriptor", "Type 0x%02X, %u bytes",
				(unsigned) descriptor->DescriptorType, (unsigned) descriptor->Length );
		}

		Add_Descriptor_Bytes( list, descriptor, decoded );

		offset += descriptor->Length;
	}

	if ( offset != length )
	{
		Add_Property( list, "Descriptor data", "Malformed or truncated at byte %u", (unsigned) offset );
	}
}

// --

static void
Add_Configuration( struct List *list, const struct USB2_FktNode *node )
{
	const struct USB2_Config_Desc *config;

	U32 available;
	U32 total;
	U32 length;
	U32 milliamps;

	config = node->CfgDesc;

	if ( ( ! config ) || ( node->CfgSize < (S32) sizeof( *config ) ) )
	{
		Add_Property( list, "Configuration descriptor", "%s", "Unavailable" );

		return;
	}

	available = node->CfgSize;
	total = LE_SWAP16( config->TotalLength );

	if ( ( config->DescriptorType != DSCTYPE_Configuration ) ||
		( config->Length < sizeof( *config ) ) || ( config->Length > available ) || ( config->Length > total ) )
	{
		Add_Property( list, "Configuration descriptor", "%s", "Malformed header" );

		return;
	}

	length = total < available ? total : available;
	milliamps = config->MaxPower * ( node->Speed == USBSPEED_Super ? 8U : 2U );

	Add_Property( list, "Configuration descriptor", "Index 0, value %u, %u header bytes, %u total bytes",
		(unsigned) config->ConfigurationValue, (unsigned) config->Length, (unsigned) total );

	Add_Property( list, "Interfaces (bNumInterfaces)", "%u", (unsigned) config->NumInterfaces );

	Add_Property( list, "Configuration string (iConfiguration)", "%u", (unsigned) config->ConfigurationStrNr );

	Add_Property( list, "Attributes (bmAttributes)", "0x%02X, %s, remote wakeup %s",
		(unsigned) config->Attributes, ( config->Attributes & 0x40 ) ? "self-powered" : "bus-powered",
		( config->Attributes & 0x20 ) ? "supported" : "unavailable" );

	if ( node->Speed <= USBSPEED_Super )
	{
		Add_Property( list, "Maximum bus power (bMaxPower)", "%u mA (0x%02X)", (unsigned) milliamps, (unsigned) config->MaxPower );
	}
	else
	{
		Add_Property( list, "Maximum bus power (bMaxPower)", "0x%02X", (unsigned) config->MaxPower );
	}

	if ( total > available )
	{
		Add_Property( list, "Configuration data", "Truncated to %u of %u bytes", (unsigned) available, (unsigned) total );
	}

	Add_Descriptor_Bytes( list, (const struct USB2_Descriptor *) config, sizeof( *config ) );

	Add_Descriptor_List( list, (const U8 *) config + config->Length, length - config->Length, FALSE );
}

// --

void
Build_Device_Properties( struct List *list, const struct USB2_FktNode *node )
{
	static const char * const speeds[] = {
		[USBSPEED_Low]		= "Low speed (1.5 Mb/s)",
		[USBSPEED_Full]		= "Full speed (12 Mb/s)",
		[USBSPEED_High]		= "High speed (480 Mb/s)",
		[USBSPEED_Super]		= "SuperSpeed",
	};

	const struct USB2_Device_Desc *device;
	const struct USB2_Interface_Desc *interface;

	U32 version;
	U32 revision;

	if ( ! node )
	{
		return;
	}

	switch( node->Type )
	{
		case TYPE_HCD:
		{
			Add_Property( list, "Host controller", "%s", node->Name ? node->Name : "Unknown" );

			break;
		}

		case TYPE_FUNCTION:
		{
			device = & node->DevDsc;
			version = LE_SWAP16( device->USBVers );
			revision = LE_SWAP16( device->DeviceVers );

			Add_Property( list, "Device descriptor", "%u bytes, type 0x%02X",
				(unsigned) device->Length, (unsigned) device->DescriptorType );

			Add_Property( list, "Vendor ID (idVendor)", "0x%04X", (unsigned) LE_SWAP16( device->VendorID ) );

			Add_Property( list, "Product ID (idProduct)", "0x%04X", (unsigned) LE_SWAP16( device->DeviceID ) );

			if ( node->ProductStr[0] )
			{
				Add_Property( list, "Product", "%s", node->ProductStr );
			}

			if ( node->ManufacturerStr[0] )
			{
				Add_Property( list, "Manufacturer", "%s", node->ManufacturerStr );
			}

			if ( node->SerialStr[0] )
			{
				Add_Property( list, "Serial number", "%s", node->SerialStr );
			}

			Add_Class( list, device->DeviceClass, device->DeviceSubClass, device->DeviceProtocol );

			if ( node->Speed < sizeof( speeds ) / sizeof( speeds[0] ) )
			{
				Add_Property( list, "Connection speed", "%s", speeds[node->Speed] );
			}

			Add_Property( list, "USB version (bcdUSB)", "%X.%02X", (unsigned) ( version >> 8 ), (unsigned) ( version & 0xff ) );

			Add_Property( list, "Device revision (bcdDevice)", "%X.%02X", (unsigned) ( revision >> 8 ), (unsigned) ( revision & 0xff ) );

			Add_Property( list, "Endpoint 0 packet field (bMaxPacketSize0)", "0x%02X", (unsigned) device->MaxPacketSize0 );

			Add_Property( list, "String indexes (manufacturer / product / serial)", "%u / %u / %u",
				(unsigned) device->ManufacturerStrNr, (unsigned) device->DeviceStrNr, (unsigned) device->SerialStrNr );

			Add_Property( list, "Configurations (bNumConfigurations)", "%u", (unsigned) device->NumConfigurations );

			Add_Property( list, "Stack function ID", "0x%08lX%08lX", (unsigned long) ( node->ID >> 32 ),
				(unsigned long) ( node->ID & 0xffffffffU ) );

			if ( node->Port )
			{
				Add_Property( list, "Hub port", "%u", (unsigned) node->Port );
			}

			if ( device->DeviceClass == USBCLASS_HUB )
			{
				Add_Property( list, "Downstream ports", "%u", (unsigned) node->Ports );
			}

			Add_Configuration( list, node );

			break;
		}

		case TYPE_INTERFACE:
		{
			interface = node->IfcDesc;

			if ( ( ! interface ) || ( node->IfcSize < (S32) sizeof( *interface ) ) ||
				( interface->Length < sizeof( *interface ) ) || ( interface->Length > node->IfcSize ) )
			{
				break;
			}

			Add_Property( list, "Configuration", "%u", (unsigned) node->Config );

			Add_Interface( list, interface );

			Add_Descriptor_Bytes( list, (const struct USB2_Descriptor *) interface, sizeof( *interface ) );

			Add_Descriptor_List( list, (const U8 *) interface + interface->Length, node->IfcSize - interface->Length, TRUE );

			break;
		}
	}
}

// --
