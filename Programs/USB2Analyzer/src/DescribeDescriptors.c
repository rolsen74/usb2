
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct DescriptorOutput
{
	DescriptorRow Emit;
	PTR Target;
	U32 Level;
};

// --

static void
Section( struct DescriptorOutput *out, U32 level, const char *title )
{
	out->Emit( out->Target, level, TRUE, title, "", "", "" );
	out->Level = level + 1;
}

// --

static void
Field( struct DescriptorOutput *out, const char *label, U32 bytes, U32 value, const char *description )
{
	char size[12];
	char hex[12];

	snprintf( size, sizeof( size ), "%u", (unsigned) bytes );

	snprintf( hex, sizeof( hex ), "0x%0*X", (int) ( bytes * 2 ), (unsigned) value );

	out->Emit( out->Target, out->Level, FALSE, label, size, hex, description );
}

// --

static void
Decimal( struct DescriptorOutput *out, const char *label, U32 bytes, U32 value, const char *description )
{
	char size[12];
	char number[12];

	snprintf( size, sizeof( size ), "%u", (unsigned) bytes );

	snprintf( number, sizeof( number ), "%u", (unsigned) value );

	out->Emit( out->Target, out->Level, FALSE, label, size, number, description );
}

// --

static void
Note( struct DescriptorOutput *out, const char *label, const char *description )
{
	out->Emit( out->Target, out->Level, FALSE, label, "", "", description );
}

// --

static U32
Read_Word( const U8 *data )
{
	return ( (U32) data[0] | ( (U32) data[1] << 8 ) );
}

// --

static const char *
Descriptor_Name( U32 type )
{
	switch( type )
	{
		case DSCTYPE_Device:                    return ( "Device descriptor" );
		case DSCTYPE_Configuration:             return ( "Configuration descriptor" );
		case DSCTYPE_String:                    return ( "String descriptor" );
		case DSCTYPE_Interface:                 return ( "Interface descriptor" );
		case DSCTYPE_EndPoint:                  return ( "Endpoint descriptor" );
		case DSCTYPE_Device_Qualifier:          return ( "Device qualifier descriptor" );
		case DSCTYPE_Other_Speed_Configuration: return ( "Other-speed configuration" );
		case DSCTYPE_Interface_Power:           return ( "Interface power descriptor" );
		case DSCTYPE_OTG:                       return ( "OTG descriptor" );
		case DSCTYPE_Debug:                     return ( "Debug descriptor" );
		case DSCTYPE_Interface_Association:     return ( "Interface association" );
		case 0x24:                              return ( "Class-specific interface descriptor" );
		case 0x25:                              return ( "Class-specific endpoint descriptor" );
		case 0x29:                              return ( "Hub descriptor" );
		case 0x2a:                              return ( "SuperSpeed hub descriptor" );
		case 0x30:                              return ( "SuperSpeed endpoint companion" );
		case 0x31:                              return ( "SuperSpeedPlus isochronous companion" );
		default:                                return ( "Descriptor" );
	}
}

// --

static void
Header( struct DescriptorOutput *out, const U8 *data, const char *name )
{
	Decimal( out, "bLength", 1, data[0], "Descriptor length in bytes" );

	Field( out, "bDescriptorType", 1, data[1], name );
}

// --

static void
Raw_Bytes( struct DescriptorOutput *out, const U8 *data, U32 start, U32 length )
{
	static const char hex[] = "0123456789ABCDEF";

	U32 offset;
	U32 count;
	U32 index;
	U32 used;
	char label[24];
	char size[12];
	char value[3 * 16];

	for( offset = start; offset < length; offset += count )
	{
		count = MIN( length - offset, 16 );
		used = 0;

		for( index = 0; index < count; index++ )
		{
			if ( index )
			{
				value[used++] = ' ';
			}

			value[used++] = hex[data[offset + index] >> 4];
			value[used++] = hex[data[offset + index] & 15];
		}

		value[used] = '\0';

		snprintf( label, sizeof( label ), "Raw +0x%04X", (unsigned) offset );

		snprintf( size, sizeof( size ), "%u", (unsigned) count );

		out->Emit( out->Target, out->Level, FALSE, label, size, value, "" );
	}
}

// --

static void
Class_Fields( struct DescriptorOutput *out, const U8 *data,
	const char *class_label, const char *subclass_label, const char *protocol_label )
{
	const char *name;

	name = data[0] ? GetClassName( data[0], data[1], data[2] ) : "Defined by interfaces";

	Field( out, class_label, 1, data[0], name ? name : "" );

	Field( out, subclass_label, 1, data[1], "" );

	Field( out, protocol_label, 1, data[2], "" );
}

// --

static void
BCD( struct DescriptorOutput *out, const char *label, U32 value )
{
	char version[24];

	if ( ( ( value & 0xf ) > 9 ) || ( ( ( value >> 4 ) & 0xf ) > 9 ) ||
		( ( ( value >> 8 ) & 0xf ) > 9 ) || ( ( value >> 12 ) > 9 ) )
	{
		Field( out, label, 2, value, "Invalid BCD" );

		return;
	}

	snprintf( version, sizeof( version ), "%X.%02X", (unsigned) ( value >> 8 ), (unsigned) ( value & 0xff ) );

	Field( out, label, 2, value, version );
}

// --

static void
Describe_String( struct DescriptorOutput *out, USB2_ID id, U32 tag, U8 index,
	const char *title, const char *value )
{
	U32 length;
	U8 data[255];

	if ( ! index )
	{
		return;
	}

	Section( out, 0, title );

	Decimal( out, "String index", 1, index, value );

	length = USB2_Attr_Get( id, tag, data, sizeof( data ) );

	if ( ( length < 2 ) || ( length > sizeof( data ) ) )
	{
		Note( out, "Raw descriptor", "Unavailable" );

		return;
	}

	Header( out, data, "String descriptor" );

	Raw_Bytes( out, data, 0, length );
}

// --

static void
Describe_Hub( struct DescriptorOutput *out, const struct USB2_FktNode *node )
{
	U8 data[sizeof( struct USB2_HUB_Desc )];
	U32 length;

	length = USB2_Attr_Get( node->ID, USB2Tag_Attr_HUBDesc, data, sizeof( data ) );

	if ( ( length < 2 ) || ( length > sizeof( data ) ) )
	{
		return;
	}

	length = MIN( length, data[0] );

	if ( length < 2 )
	{
		return;
	}

	Section( out, 0, Descriptor_Name( data[1] ) );

	Header( out, data, Descriptor_Name( data[1] ) );

	if ( length >= 7 )
	{
		Decimal( out, "bNbrPorts", 1, data[2], "Downstream ports" );

		Field( out, "wHubCharacteristics", 2, Read_Word( data + 3 ), "" );

		Decimal( out, "bPwrOn2PwrGood", 1, data[5], "Power-on delay in 2 ms units" );

		Decimal( out, "bHubContrCurrent", 1, data[6], "Hub controller current field" );
	}

	if ( ( data[1] == 0x2a ) && ( length >= 10 ) )
	{
		Decimal( out, "bHubHdrDecLat", 1, data[7], "Header decode latency" );

		Field( out, "wHubDelay", 2, Read_Word( data + 8 ), "" );
	}

	Raw_Bytes( out, data, 0, length );
}

// --

static void
Describe_Device( struct DescriptorOutput *out, const struct USB2_FktNode *node )
{
	static const char * const speeds[] =
	{
		[USBSPEED_Low] = "Low speed (1.5 Mb/s)",
		[USBSPEED_Full] = "Full speed (12 Mb/s)",
		[USBSPEED_High] = "High speed (480 Mb/s)",
		[USBSPEED_Super] = "SuperSpeed",
	};

	const U8 *data;

	char id[24];

	data = (const U8 *) & node->DevDsc;

	Section( out, 0, "Device descriptor" );

	Header( out, data, "Device descriptor" );

	BCD( out, "bcdUSB", Read_Word( data + 2 ) );

	Class_Fields( out, data + 4, "bDeviceClass", "bDeviceSubClass", "bDeviceProtocol" );

	Decimal( out, "bMaxPacketSize0", 1, data[7], "Endpoint 0 packet size field" );

	Field( out, "idVendor", 2, Read_Word( data + 8 ), node->ManufacturerStr );

	Field( out, "idProduct", 2, Read_Word( data + 10 ), node->ProductStr );

	BCD( out, "bcdDevice", Read_Word( data + 12 ) );

	Decimal( out, "iManufacturer", 1, data[14], node->ManufacturerStr );

	Decimal( out, "iProduct", 1, data[15], node->ProductStr );

	Decimal( out, "iSerialNumber", 1, data[16], node->SerialStr );

	Decimal( out, "bNumConfigurations", 1, data[17], "" );

	if ( ( node->Speed < sizeof( speeds ) / sizeof( speeds[0] ) ) && ( speeds[node->Speed] ) )
	{
		Note( out, "Connection speed", speeds[node->Speed] );
	}

	snprintf( id, sizeof( id ), "0x%08lX%08lX", (unsigned long) ( node->ID >> 32 ),
		(unsigned long) ( node->ID & 0xffffffffU ) );

	Note( out, "Stack function ID", id );

	if ( ( data[0] != sizeof( node->DevDsc ) ) || ( data[1] != DSCTYPE_Device ) )
	{
		Raw_Bytes( out, data, 0, sizeof( node->DevDsc ) );
	}

	if ( data[4] == USBCLASS_HUB )
	{
		Describe_Hub( out, node );
	}

	Describe_String( out, node->ID, USB2Tag_Attr_ManufacturerStr, data[14], "Manufacturer string", node->ManufacturerStr );

	Describe_String( out, node->ID, USB2Tag_Attr_ProductStr, data[15], "Product string", node->ProductStr );

	Describe_String( out, node->ID, USB2Tag_Attr_SerialStr, data[16], "Serial number string", node->SerialStr );
}

// --

static void
Config_Fields( struct DescriptorOutput *out, const U8 *data, U32 speed )
{
	char attributes[64];
	char power[24];

	snprintf( attributes, sizeof( attributes ), "%s%s",
		( data[7] & 0x40 ) ? "Self-powered" : "Bus-powered",
		( data[7] & 0x20 ) ? ", remote wakeup" : "" );

	power[0] = '\0';

	if ( speed <= USBSPEED_Super )
	{
		snprintf( power, sizeof( power ), "%u mA", (unsigned) data[8] * ( speed == USBSPEED_Super ? 8U : 2U ) );
	}

	Decimal( out, "wTotalLength", 2, Read_Word( data + 2 ), "Configuration length in bytes" );

	Decimal( out, "bNumInterfaces", 1, data[4], "" );

	Decimal( out, "bConfigurationValue", 1, data[5], "" );

	Decimal( out, "iConfiguration", 1, data[6], "Configuration string index" );

	Field( out, "bmAttributes", 1, data[7], attributes );

	Field( out, "bMaxPower", 1, data[8], power );
}

// --

static void
Interface_Fields( struct DescriptorOutput *out, const U8 *data )
{
	Decimal( out, "bInterfaceNumber", 1, data[2], "" );

	Decimal( out, "bAlternateSetting", 1, data[3], "" );

	Decimal( out, "bNumEndpoints", 1, data[4], "" );

	Class_Fields( out, data + 5, "bInterfaceClass", "bInterfaceSubClass", "bInterfaceProtocol" );

	Decimal( out, "iInterface", 1, data[8], "Interface string index" );
}

// --

static void
Endpoint_Fields( struct DescriptorOutput *out, const U8 *data )
{
	static const char * const types[] = { "Control", "Isochronous", "Bulk", "Interrupt" };

	U32 packet;
	char description[32];

	packet = Read_Word( data + 4 );

	Field( out, "bEndpointAddress", 1, data[2], ( data[2] & EPADR_Dir_In ) ? "IN" : "OUT" );

	Field( out, "bmAttributes", 1, data[3], types[data[3] & EPATT_Type_Mask] );

	snprintf( description, sizeof( description ), "%u bytes per packet", (unsigned) ( packet & 0x07ff ) );

	Field( out, "wMaxPacketSize", 2, packet, description );

	Decimal( out, "bInterval", 1, data[6], "Polling interval field" );
}

// --

static void
Association_Fields( struct DescriptorOutput *out, const U8 *data )
{
	Decimal( out, "bFirstInterface", 1, data[2], "" );

	Decimal( out, "bInterfaceCount", 1, data[3], "" );

	Class_Fields( out, data + 4, "bFunctionClass", "bFunctionSubClass", "bFunctionProtocol" );

	Decimal( out, "iFunction", 1, data[7], "Function string index" );
}

// --

static void
HID_Fields( struct DescriptorOutput *out, const U8 *data, U32 length )
{
	U32 offset;
	U32 index;

	BCD( out, "bcdHID", Read_Word( data + 2 ) );

	Decimal( out, "bCountryCode", 1, data[4], "" );

	Decimal( out, "bNumDescriptors", 1, data[5], "Number of subordinate descriptors" );

	for( index = 0, offset = 6; ( index < data[5] ) && ( length - offset >= 3 ); index++, offset += 3 )
	{
		Field( out, "bDescriptorType", 1, data[offset], data[offset] == 0x22 ? "Report descriptor" : "" );

		Decimal( out, "wDescriptorLength", 2, Read_Word( data + offset + 1 ), "Subordinate descriptor length in bytes" );
	}

	if ( index != data[5] )
	{
		Note( out, "Descriptor entries", "Truncated" );
	}
}

// --

static void
Describe_Stream( struct DescriptorOutput *out, const U8 *data, U32 size, U32 speed, S32 interface_only )
{
	const U8 *descriptor;
	const char *name;

	U32 offset;
	U32 length;
	U32 type;
	U32 level;
	U32 child_level;
	U32 first_interface;
	U32 last_interface;
	U32 interface_class;
	U32 total;
	U32 decoded;
	S32 hid;
	char title[96];
	char description[64];

	offset = 0;
	child_level = 1;
	first_interface = 0;
	last_interface = 0;
	interface_class = 0;

	while( offset < size )
	{
		descriptor = data + offset;
		length = descriptor[0];

		if ( ( length < 2 ) || ( length > size - offset ) )
		{
			snprintf( title, sizeof( title ), "Malformed descriptor at +0x%04X", (unsigned) offset );

			Section( out, child_level, title );

			Raw_Bytes( out, descriptor, 0, size - offset );

			return;
		}

		type = descriptor[1];

		if ( ( interface_only ) && ( offset ) && ( ( type == DSCTYPE_Interface ) ||
			( type == DSCTYPE_Interface_Association ) || ( type == DSCTYPE_Configuration ) ) )
		{
			return;
		}

		hid = ( type == 0x21 ) && ( interface_class == USBCLASS_HID );
		name = hid ? "HID descriptor" : Descriptor_Name( type );
		level = child_level;

		snprintf( title, sizeof( title ), "%s [0x%02X]", name, (unsigned) type );

		switch( type )
		{
			case DSCTYPE_Configuration:
			case DSCTYPE_Other_Speed_Configuration:
				level = 0;
				child_level = 1;
				last_interface = 0;
				interface_class = 0;

				if ( length >= 9 )
				{
					snprintf( title, sizeof( title ), "%s %u", name, (unsigned) descriptor[5] );
				}
				break;

			case DSCTYPE_Interface_Association:
				level = 1;
				child_level = 2;
				last_interface = 0;
				interface_class = 0;

				if ( length >= 8 )
				{
					first_interface = descriptor[2];
					last_interface = first_interface + descriptor[3];
				}
				break;

			case DSCTYPE_Interface:
				interface_class = ( length >= 9 ) ? descriptor[5] : 0;
				level = interface_only ? 0 : 1;

				if ( length >= 9 )
				{
					if ( ( descriptor[2] >= first_interface ) && ( descriptor[2] < last_interface ) )
					{
						level++;
					}

					snprintf( title, sizeof( title ), "Interface %u, alternate %u",
						(unsigned) descriptor[2], (unsigned) descriptor[3] );
				}

				child_level = level + 1;
				break;

			case DSCTYPE_EndPoint:
				if ( length >= 7 )
				{
					snprintf( title, sizeof( title ), "Endpoint 0x%02X %s", (unsigned) descriptor[2],
						( descriptor[2] & EPADR_Dir_In ) ? "IN" : "OUT" );
				}
				break;
		}

		Section( out, level, title );

		Header( out, descriptor, name );

		decoded = 0;

		if ( ( ( type == DSCTYPE_Configuration ) || ( type == DSCTYPE_Other_Speed_Configuration ) ) && ( length >= 9 ) )
		{
			Config_Fields( out, descriptor, speed );

			total = Read_Word( descriptor + 2 );

			if ( total > size - offset )
			{
				snprintf( description, sizeof( description ), "%u of %u bytes cached",
					(unsigned) ( size - offset ), (unsigned) total );

				Note( out, "Configuration stream", description );
			}
		}
		else if ( ( type == DSCTYPE_Interface ) && ( length >= 9 ) )
		{
			Interface_Fields( out, descriptor );

			decoded = 9;
		}
		else if ( ( type == DSCTYPE_EndPoint ) && ( length >= 7 ) )
		{
			Endpoint_Fields( out, descriptor );

			decoded = 7;
		}
		else if ( ( type == DSCTYPE_Interface_Association ) && ( length >= 8 ) )
		{
			Association_Fields( out, descriptor );
		}
		else if ( ( hid ) && ( length >= 6 ) )
		{
			HID_Fields( out, descriptor, length );
		}

		Raw_Bytes( out, descriptor, decoded, length );

		offset += length;
	}
}

// --

void
Describe_Descriptors( const struct USB2_FktNode *node, DescriptorRow emit, PTR target )
{
	struct DescriptorOutput out;

	if ( ! node )
	{
		return;
	}

	out.Emit = emit;
	out.Target = target;
	out.Level = 0;

	switch( node->Type )
	{
		case TYPE_HCD:
			Note( & out, "Host controller", node->Name ? node->Name : "Unknown" );
			break;

		case TYPE_FUNCTION:
			Describe_Device( & out, node );

			if ( ( node->CfgDesc ) && ( node->CfgSize > 0 ) )
			{
				Describe_Stream( & out, (const U8 *) node->CfgDesc, node->CfgSize, node->Speed, FALSE );
			}
			else
			{
				out.Level = 0;

				Note( & out, "Configuration descriptors", "Unavailable" );
			}
			break;

		case TYPE_INTERFACE:
			if ( ( node->IfcDesc ) && ( node->IfcSize > 0 ) )
			{
				Describe_Stream( & out, (const U8 *) node->IfcDesc, node->IfcSize, node->Speed, TRUE );
			}
			break;
	}
}

// --
