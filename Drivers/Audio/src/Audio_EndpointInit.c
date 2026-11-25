
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_CODE static STR
AppendHex( STR text, U32 value, U32 digits )
{
	static const char hex[] = "0123456789abcdef";

	while( digits )
	{
		digits--;
		*text++ = hex[( value >> ( digits * 4 ) ) & 15];
	}

	*text = 0;

	return ( text );
}

// --

SEC_CODE static U32
ReadString( USB2_ID id, U32 tag, U8 *buffer, U32 size )
{
	U32 actual;
	U32 length;

	actual = USB2_Attr_Get( id, tag, buffer, size );

	if ( actual < 4 )
	{
		return ( 0 );
	}

	length = buffer[0];

	if ( ( length < 4 ) || ( length > actual ) || ( length & 1 ) || ( buffer[1] != DSCTYPE_String ) )
	{
		return ( 0 );
	}

	return ( length );
}

// --

SEC_CODE void
Audio_EndpointInit( struct AudioUnit *unit )
{
	struct AudioEndpoint *endpoint;
	struct USB2_Config_Desc *config;
	struct USB2_PortPath path;

	USB2_ID id;
	U8 string[255];
	char prefix[AUDIO_ENDPOINT_ID_SIZE];
	STR text;
	U64 hash;
	U32 length;
	U32 index;
	U32 used;
	U16 code;
	S32 usable;

	endpoint = & unit->Endpoint;
	config = (PTR) unit->StartMsg->ConfigDescriptors;
	id = unit->StartMsg->Function->FKT_ID;
	used = 0;
	usable = FALSE;
	hash = 14695981039346656037ULL;
	endpoint->DefaultFormat = unit->Format;
	endpoint->Direction = ( unit->EndPoint & EPADR_Dir_Mask ) ? AUDIO_ENDPOINT_INPUT : AUDIO_ENDPOINT_OUTPUT;
	endpoint->Transport = AUDIO_TRANSPORT_USB;

	Audio_EndpointRates( unit );

	length = ReadString( id, USB2Tag_Attr_ProductStr, string, sizeof( string ) );

	for( index = 2; ( index < length ) && ( used + 1 < sizeof( endpoint->Name ) ); index += 2 )
	{
		code = string[index] | ( (U16) string[index + 1] << 8 );

		if ( ! code )
		{
			break;
		}

		endpoint->Name[used++] = ( ( code >= 32 && code < 127 ) || ( code >= 160 && code <= 255 ) ) ? code : '?';
	}

	if ( ! used )
	{
		CopyMem( "USB Audio", endpoint->Name, sizeof( "USB Audio" ) );
	}

	CopyMem( "usb-", prefix, sizeof( "usb-" ) );

	text = AppendHex( prefix + 4, unit->Format.VendorID, 4 );

	*text++ = '-';

	text = AppendHex( text, unit->Format.DeviceID, 4 );

	*text++ = '-';

	text = AppendHex( text, config->ConfigurationValue, 2 );

	text = AppendHex( text, unit->InterfaceNr, 2 );

	*text++ = ( endpoint->Direction == AUDIO_ENDPOINT_INPUT ) ? 'i' : 'o';
	*text++ = '-';

	used = text - prefix;

	length = ReadString( id, USB2Tag_Attr_SerialStr, string, sizeof( string ) );

	for( index = 2; index < length; index += 2 )
	{
		code = string[index] | ( (U16) string[index + 1] << 8 );

		if ( ! code )
		{
			usable = FALSE;
			break;
		}

		usable |= ( code != ' ' );
		hash = ( hash ^ string[index] ) * 1099511628211ULL;
		hash = ( hash ^ string[index + 1] ) * 1099511628211ULL;
	}

	if ( usable )
	{
		CopyMem( prefix, endpoint->ID, used );

		text = endpoint->ID + used;
		*text++ = 's';

		text = AppendHex( text, (U32) ( hash >> 32 ), 8 );

		AppendHex( text, (U32) hash, 8 );
	}

	if ( ( USB2_Attr_Get( id, USB2Tag_Attr_PortPath, & path, sizeof( path ) ) == sizeof( path ) ) &&
		( path.Depth ) && ( path.Depth <= USB2_PORT_PATH_DEPTH ) )
	{
		CopyMem( prefix, unit->PortID, used );

		text = unit->PortID + used;
		*text++ = path.IsPCI ? 'p' : 'm';

		text = AppendHex( text, path.Controller, 8 );

		for( index = 0; index < path.Depth; index++ )
		{
			*text++ = '.';

			text = AppendHex( text, path.Ports[index], 2 );
		}
	}
}

// --
