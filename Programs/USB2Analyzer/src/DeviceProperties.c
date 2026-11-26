
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

struct PropertyList
{
	struct List *Rows;
	struct TextFont *Font;
	struct DrawInfo *DrawInfo;
	Object *Image;
	U32 Level;
	S32 InDescriptor;
};

// --

static S32
Add_Property_Text( struct List *list, U32 level, S32 heading, const char *text )
{
	struct Node *row;

	row = AllocListBrowserNode( 1,
		LBNA_Generation, level + 1,
		LBNA_Flags, heading ? LBFLG_HASCHILDREN | LBFLG_SHOWCHILDREN : LBFLG_READONLY,
		LBNA_Column, 0,
		 LBNCA_CopyText, TRUE,
		 LBNCA_Text, text,
		TAG_END
	);

	if ( ! row )
	{
		return ( FALSE );
	}

	AddTail( list, row );

	return ( TRUE );
}

// --

static void
Flush_Property_Image( struct PropertyList *properties )
{
	struct Node *row;

	if ( ! properties->Image )
	{
		return;
	}

	row = AllocListBrowserNode( 1,
		LBNA_Generation, properties->Level + 1,
		LBNA_Flags, LBFLG_READONLY,
		LBNA_UserData, properties->Image,
		LBNA_Column, 0,
		 LBNCA_Image, properties->Image,
		TAG_END
	);

	if ( row )
	{
		AddTail( properties->Rows, row );
	}
	else
	{
		DisposeObject( properties->Image );
	}

	properties->Image = NULL;
}

// --

static void
Add_Property_Row( PTR target, U32 level, S32 heading, const char *field, const char *size,
	const char *value, const char *description )
{
	struct PropertyList *properties;

	char text[512];
	const char *display;

	properties = target;

	if ( heading )
	{
		Flush_Property_Image( properties );

		properties->InDescriptor = Add_Property_Text( properties->Rows, level, TRUE, field );
		properties->Level = level + 1;

		return;
	}

	display = *value ? value : description;

	// The size and value already describe bLength.
	if ( ( *value ) && ( *description ) && ( strcmp( field, "bLength" ) ) )
	{
		if ( ( description[0] == 'v' ) && ( ! strncmp( field, "bcd", 3 ) ) )
		{
			snprintf( text, sizeof( text ), "%s (%s)", description, value );
		}
		else
		{
			snprintf( text, sizeof( text ), "%s (%s)", value, description );
		}

		display = text;
	}

	if ( ( properties->InDescriptor ) && ( level == properties->Level ) )
	{
		if ( properties->Image )
		{
			if ( Append_Descriptor_Image( properties->Image, field, size, display ) )
			{
				return;
			}

			Flush_Property_Image( properties );
		}

		properties->Image = New_Descriptor_Image( properties->Font, properties->DrawInfo );

		if ( properties->Image )
		{
			if ( Append_Descriptor_Image( properties->Image, field, size, display ) )
			{
				return;
			}

			DisposeObject( properties->Image );

			properties->Image = NULL;
		}
	}

	Flush_Property_Image( properties );

	if ( *display )
	{
		char line[640];

		snprintf( line, sizeof( line ), "%s: %s", field, display );

		Add_Property_Text( properties->Rows, level, FALSE, line );
	}
	else
	{
		Add_Property_Text( properties->Rows, level, FALSE, field );
	}
}

// --

void
Build_Device_Properties( struct List *list, const struct USB2_FktNode *node, struct Window *window )
{
	struct PropertyList properties;

	if ( ( ! node ) || ( ! window ) )
	{
		return;
	}

	memset( &properties, 0, sizeof( properties ) );

	properties.Rows = list;
	properties.Font = window->RPort->Font;

	properties.DrawInfo = GetScreenDrawInfo( window->WScreen );

	Describe_Descriptors( node, Add_Property_Row, &properties );

	Flush_Property_Image( &properties );

	if ( properties.DrawInfo )
	{
		FreeScreenDrawInfo( window->WScreen, properties.DrawInfo );
	}
}

// --

void
Free_Device_Properties( struct List *list )
{
	struct Node *row;

	Object *image;

	while( ( row = RemHead( list ) ) )
	{
		image = NULL;

		GetListBrowserNodeAttrs( row, LBNA_UserData, &image, TAG_END );

		FreeListBrowserNode( row );

		if ( image )
		{
			DisposeObject( image );
		}
	}
}

// --
