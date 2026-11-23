
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

enum ClassField
{
	FIELD_Title,
	FIELD_Driver,
	FIELD_Alias,
	FIELD_Type,
	FIELD_Class,
	FIELD_SubClass,
	FIELD_Protocol,
	FIELD_Device,
	FIELD_Count
};

static const struct
{
	STR Name;
	U32 Tag;
} ClassFields[FIELD_Count] =
{
	[FIELD_Title]    = { "TITLE",    USB2Tag_FDriver_Title },
	[FIELD_Driver]   = { "DRIVER",   USB2Tag_FDriver_Driver_Filename },
	[FIELD_Alias]    = { "ALIAS",    USB2Tag_FDriver_Alias },
	[FIELD_Type]     = { "TYPE",     USB2Tag_FDriver_Type },
	[FIELD_Class]    = { "CLASS",    USB2Tag_FDriver_Class },
	[FIELD_SubClass] = { "SUBCLASS", USB2Tag_FDriver_SubClass },
	[FIELD_Protocol] = { "PROTOCOL", USB2Tag_FDriver_Protocol },
	[FIELD_Device]   = { "DEVICE",   USB2Tag_FDriver_DeviceList }
};

// --

SEC_CODE static STR
SkipSpace( STR pos )
{
	while ( ( *pos == ' ' ) || ( *pos == '\t' ) )
	{
		pos++;
	}

	return ( pos );
}

// --

SEC_CODE static S32
ReadNumber( STR *input, U32 *number, U32 max )
{
	STR pos;
	U32 base;
	U32 value;
	U32 digits;
	U32 digit;

	pos = *input;
	base = 10;
	value = 0;
	digits = 0;

	if ( ( pos[0] == '0' ) && ( ( pos[1] == 'x' ) || ( pos[1] == 'X' ) ) )
	{
		base = 16;
		pos += 2;
	}

	for ( ; *pos; pos++ )
	{
		if ( ( *pos >= '0' ) && ( *pos <= '9' ) )
		{
			digit = *pos - '0';
		}
		else if ( ( *pos >= 'a' ) && ( *pos <= 'f' ) )
		{
			digit = *pos - 'a' + 10;
		}
		else if ( ( *pos >= 'A' ) && ( *pos <= 'F' ) )
		{
			digit = *pos - 'A' + 10;
		}
		else
		{
			break;
		}

		if ( digit >= base )
		{
			break;
		}

		if ( value > ( max - digit ) / base )
		{
			return ( FALSE );
		}

		value = value * base + digit;
		digits++;
	}

	if ( ! digits )
	{
		return ( FALSE );
	}

	*input = pos;
	*number = value;

	return ( TRUE );
}

// --

SEC_CODE static S32
ReadDevice( struct USBBase *usbbase, struct List *devices, STR pos )
{
	struct USB2_DeviceNode *device;

	U32 vendor;
	U32 product;

	product = 0;

	if ( ! ReadNumber( &pos, &vendor, 0xffff ) )
	{
		return ( FALSE );
	}

	pos = SkipSpace( pos );

	if ( *pos == ':' )
	{
		pos = SkipSpace( pos + 1 );

		if ( ! ReadNumber( &pos, &product, 0xffff ) )
		{
			return ( FALSE );
		}

		pos = SkipSpace( pos );
	}

	if ( ( *pos ) && ( *pos != '#' ) )
	{
		return ( FALSE );
	}

	device = MEM_ALLOCVEC( sizeof( *device ), TRUE );

	if ( ! device )
	{
		return ( FALSE );
	}

	device->VendorID = vendor;
	device->DeviceID = product;

	usbbase->usb_IExec->AddTail( devices, &device->Node );

	return ( TRUE );
}

// --

SEC_CODE static STR
ReadText( STR pos )
{
	STR text;
	STR end;

	U8 quote;

	quote = *pos;

	if ( ( quote == '\'' ) || ( quote == '"' ) || ( quote == '`' ) )
	{
		text = ++pos;

		while ( ( *pos ) && ( *pos != quote ) )
		{
			pos++;
		}

		if ( ( ! *pos ) || ( pos == text ) )
		{
			return ( NULL );
		}

		end = SkipSpace( pos + 1 );

		if ( ( *end ) && ( *end != '#' ) )
		{
			return ( NULL );
		}
	}
	else
	{
		text = pos;

		while ( ( *pos ) && ( *pos != '#' ) )
		{
			pos++;
		}

		while ( ( pos > text ) && ( ( pos[-1] == ' ' ) || ( pos[-1] == '\t' ) ) )
		{
			pos--;
		}

		if ( pos == text )
		{
			return ( NULL );
		}
	}

	*pos = 0;

	return ( text );
}

// --

SEC_CODE static S32
ParseLine( struct USBBase *usbbase, struct TagItem *tags, struct List *devices, STR line )
{
	STR key;
	STR pos;
	STR text;

	U32 field;
	U32 number;
	Tag value;
	U8 separator;

	key = SkipSpace( line );

	if ( ( ! *key ) || ( *key == '#' ) )
	{
		return ( TRUE );
	}

	pos = key;

	while ( ( *pos ) && ( *pos != ' ' ) && ( *pos != '\t' ) )
	{
		pos++;
	}

	separator = *pos;
	*pos = 0;

	for ( field = 0; field < FIELD_Count; field++ )
	{
		if ( ! MISC_STRICASECOMPARE( key, ClassFields[field].Name ) )
		{
			break;
		}
	}

	if ( ( field == FIELD_Count ) || ( ! separator ) )
	{
		return ( FALSE );
	}

	pos = SkipSpace( pos + 1 );

	switch ( field )
	{
		case FIELD_Device:
		{
			return ( ReadDevice( usbbase, devices, pos ) );
		}

		case FIELD_Class:
		case FIELD_SubClass:
		case FIELD_Protocol:
		{
			if ( ! ReadNumber( &pos, &number, 0xffff ) )
			{
				return ( FALSE );
			}

			pos = SkipSpace( pos );

			if ( ( ( *pos ) && ( *pos != '#' ) ) || ( ( number > 0xff ) && ( number != 0xffff ) ) )
			{
				return ( FALSE );
			}

			value = number;
			break;
		}

		default:
		{
			text = ReadText( pos );

			if ( ! text )
			{
				return ( FALSE );
			}

			value = (Tag) text;

			if ( field == FIELD_Type )
			{
				if ( ! MISC_STRICASECOMPARE( text, "Interface" ) )
				{
					value = USB2Val_FDriver_Interface;
				}
				else if ( ! MISC_STRICASECOMPARE( text, "Function" ) )
				{
					value = USB2Val_FDriver_Function;
				}
				else if ( ! MISC_STRICASECOMPARE( text, "Device" ) )
				{
					value = USB2Val_FDriver_Device;
				}
				else
				{
					return ( FALSE );
				}
			}

			break;
		}
	}

	tags[field].ti_Tag = ClassFields[field].Tag;
	tags[field].ti_Data = value;

	return ( TRUE );
}

// --

SEC_CODE struct USB2_FktDriverNode *
__FDriver_Load( struct USBBase *usbbase, STR filename )
{
	struct USB2_FktDriverNode *fdn;
	struct TagItem tags[FIELD_Count + 2];
	struct List devices;
	struct Node *device;

	STR mem;
	STR pos;
	STR line;
	STR end;
	U32 size;
	U32 field;
	U32 line_number;
	U8 separator;

	if ( ( ! usbbase->usb_IDOS ) || ( ! filename ) )
	{
		return ( NULL );
	}

	// The directory scanner supplies the current directory.
	mem = MISC_LOADFILE( filename, &size );

	if ( ! mem )
	{
		usbbase->usb_IExec->DebugPrintF( "USB2: Could not read class %s\n", filename );

		return ( NULL );
	}

	fdn = NULL;
	pos = mem;
	end = mem + size;
	line_number = 1;

	usbbase->usb_IExec->NewList( &devices );

	for ( field = 0; field < FIELD_Count; field++ )
	{
		tags[field].ti_Tag = TAG_IGNORE;
		tags[field].ti_Data = 0;
	}

	tags[FIELD_Device].ti_Tag = USB2Tag_FDriver_DeviceList;
	tags[FIELD_Device].ti_Data = (Tag) &devices;
	tags[FIELD_Count].ti_Tag = USB2Tag_FDriver_Class_Filename;
	tags[FIELD_Count].ti_Data = (Tag) filename;
	tags[FIELD_Count + 1].ti_Tag = TAG_END;
	tags[FIELD_Count + 1].ti_Data = 0;

	while ( pos < end )
	{
		line = pos;

		while ( ( pos < end ) && ( *pos ) && ( *pos != '\r' ) && ( *pos != '\n' ) )
		{
			pos++;
		}

		if ( ( pos < end ) && ( ! *pos ) )
		{
			goto invalid;
		}

		separator = *pos;
		*pos = 0;

		if ( ! ParseLine( usbbase, tags, &devices, line ) )
		{
			goto invalid;
		}

		if ( pos < end )
		{
			pos++;

			if ( ( separator == '\r' ) && ( pos < end ) && ( *pos == '\n' ) )
			{
				pos++;
			}
		}

		line_number++;
	}

	if ( tags[FIELD_Driver].ti_Tag == TAG_IGNORE )
	{
		usbbase->usb_IExec->DebugPrintF( "USB2: Class %s is missing DRIVER\n", filename );

		goto bailout;
	}

	fdn = FDRIVER_CREATELIST( tags );

	if ( ! fdn )
	{
		usbbase->usb_IExec->DebugPrintF( "USB2: Could not register class %s\n", filename );
	}

	goto bailout;

invalid:

	usbbase->usb_IExec->DebugPrintF( "USB2: Invalid class directive in %s at line %lu\n", filename, line_number );

bailout:

	while ( ( device = usbbase->usb_IExec->RemHead( &devices ) ) )
	{
		MEM_FREEVEC( device );
	}

	MEM_FREEVEC( mem );

	return ( fdn );
}

// --
