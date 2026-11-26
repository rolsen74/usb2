
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"
#include <graphics/gfxmacros.h>
#include <graphics/rpattr.h>
#include <intuition/imageclass.h>

// --

enum
{
	TABLE_COLUMNS = 3,
	CELL_PADDING = 5,
	ROW_PADDING = 2
};

struct DescriptorImageRow
{
	struct DescriptorImageRow *Next;
	const char *Cells[TABLE_COLUMNS];
	char Text[];
};

struct DescriptorImage
{
	struct DescriptorImageRow *First;
	struct DescriptorImageRow *Last;
	struct TextFont *Font;
	U32 Columns[TABLE_COLUMNS];
	U32 RowHeight;
	U16 TextPen;
	U16 GridPen;
	U16 BodyPen;
	U16 HeaderPen;
	U16 HeaderTextPen;
};

static Class *DescriptorImageClass;

// --

static void
Set_Table_Pen( struct RastPort *rp, S32 trueColor, U32 color, U16 pen )
{
	if ( trueColor )
	{
		SetRPAttrs( rp, RPTAG_APenColor, color, TAG_END );
	}
	else
	{
		SetAPen( rp, pen );
	}
}

// --

static void
Draw_Descriptor_Image( struct Image *image, const struct DescriptorImage *table, const struct impDraw *draw )
{
	const struct DescriptorImageRow *row;
	struct RastPort rp;

	S32 left;
	S32 right;
	S32 top;
	S32 bottom;
	S32 x;
	S32 y;
	S32 offset;
	U32 length;
	U32 column;
	U32 index;
	S32 baseline;
	S32 trueColor;

	rp = *draw->imp_RPort;
	left = draw->imp_Offset.X + image->LeftEdge;
	top = draw->imp_Offset.Y + image->TopEdge;
	right = left + image->Width - 1;
	bottom = top + image->Height - 1;

	trueColor = ( rp.BitMap ) && ( GetBitMapAttr( rp.BitMap, BMA_DEPTH ) > 8 );

	SetFont( &rp, table->Font );

	SetDrMd( &rp, JAM1 );

	SetDrPt( &rp, 0xffff );

	SetAfPt( &rp, NULL, 0 );

	y = top;
	index = 0;

	for( row = table->First; row; row = row->Next )
	{
		if ( ! index )
		{
			Set_Table_Pen( &rp, trueColor, 0xff315575, table->HeaderPen );
		}
		else
		{
			Set_Table_Pen( &rp, trueColor, ( index & 1 ) ? 0xffedf4fa : 0xfff8fbfe, table->BodyPen );
		}

		RectFill( &rp, left, y, right, y + table->RowHeight - 1 );

		Set_Table_Pen( &rp, trueColor, index ? 0xff22364b : 0xfff8fbfe,
			index ? table->TextPen : table->HeaderTextPen );

		x = left;
		baseline = y + ROW_PADDING + table->Font->tf_Baseline;

		for( column = 0; column < TABLE_COLUMNS; column++ )
		{
			length = strlen( row->Cells[column] );

			if ( length )
			{
				offset = CELL_PADDING;

				if ( column == 1 )
				{
					offset = ( table->Columns[column] - TextLength( &rp, row->Cells[column], length ) ) / 2;
				}

				Move( &rp, x + offset, baseline );

				Text( &rp, row->Cells[column], length );
			}

			x += table->Columns[column];
		}

		y += table->RowHeight;
		index++;
	}

	Set_Table_Pen( &rp, trueColor, 0xffa4b8c9, table->GridPen );

	x = left;

	for( column = 0; column <= TABLE_COLUMNS; column++ )
	{
		Move( &rp, x, top );

		Draw( &rp, x, bottom );

		if ( column < TABLE_COLUMNS )
		{
			x += table->Columns[column];
		}
	}

	for( y = top; y <= bottom; y += table->RowHeight )
	{
		Move( &rp, left, y );

		Draw( &rp, right, y );
	}
}

// --

static ULONG
Descriptor_Image_Dispatch( struct Hook *hook, APTR object, APTR message )
{
	Class *class;
	struct DescriptorImage *table;
	struct DescriptorImageRow *row;
	Msg msg;

	class = (Class *) hook;
	msg = message;

	switch( msg->MethodID )
	{
		case OM_NEW:
		{
			object = (APTR) IDoSuperMethodA( class, object, msg );

			if ( object )
			{
				table = INST_DATA( class, object );

				memset( table, 0, sizeof( *table ) );
			}

			return ( (ULONG) object );
		}

		case IM_DRAW:
		case IM_DRAWFRAME:
		{
			table = INST_DATA( class, object );

			Draw_Descriptor_Image( object, table, message );

			return ( 1 );
		}

		case OM_DISPOSE:
		{
			table = INST_DATA( class, object );

			while( table->First )
			{
				row = table->First;
				table->First = row->Next;

				FreeVec( row );
			}

			break;
		}
	}

	return ( IDoSuperMethodA( class, object, msg ) );
}

// --

S32
Open_Descriptor_Image_Class( void )
{
	DescriptorImageClass = MakeClass( NULL, IMAGECLASS, NULL, sizeof( struct DescriptorImage ), 0 );

	if ( ! DescriptorImageClass )
	{
		return ( FALSE );
	}

	DescriptorImageClass->cl_Dispatcher.h_Entry = Descriptor_Image_Dispatch;

	return ( TRUE );
}

// --

void
Close_Descriptor_Image_Class( void )
{
	if ( DescriptorImageClass )
	{
		FreeClass( DescriptorImageClass );

		DescriptorImageClass = NULL;
	}
}

// --

Object *
New_Descriptor_Image( struct TextFont *font, const struct DrawInfo *draw )
{
	struct DescriptorImage *table;

	Object *object;

	if ( ! font )
	{
		return ( NULL );
	}

	object = NewObject( DescriptorImageClass, NULL, IA_Width, 1, IA_Height, 1, TAG_END );

	if ( ! object )
	{
		return ( NULL );
	}

	table = INST_DATA( DescriptorImageClass, object );
	table->Font = font;
	table->RowHeight = font->tf_YSize + ROW_PADDING * 2 + 1;
	table->TextPen = draw ? draw->dri_Pens[TEXTPEN] : 1;
	table->GridPen = draw ? draw->dri_Pens[SHADOWPEN] : 1;
	table->BodyPen = draw ? draw->dri_Pens[BACKGROUNDPEN] : 0;
	table->HeaderPen = draw ? draw->dri_Pens[FILLPEN] : 1;
	table->HeaderTextPen = draw ? draw->dri_Pens[FILLTEXTPEN] : 0;

	if ( ! Append_Descriptor_Image( object, "Field", "Size", "Value" ) )
	{
		DisposeObject( object );

		return ( NULL );
	}

	return ( object );
}

// --

S32
Append_Descriptor_Image( Object *object, const char *field, const char *size, const char *value )
{
	struct DescriptorImage *table;
	struct DescriptorImageRow *row;
	struct Image *image;
	struct RastPort rp;

	const char *cells[TABLE_COLUMNS];
	U32 widths[TABLE_COLUMNS];
	U32 lengths[TABLE_COLUMNS];
	char *text;
	U32 width;
	U32 bytes;
	U32 column;

	image = (struct Image *) object;
	table = INST_DATA( DescriptorImageClass, object );
	cells[0] = field;
	cells[1] = size;
	cells[2] = value;
	width = 1;
	bytes = sizeof( *row );

	if ( image->Height + table->RowHeight > INT16_MAX )
	{
		return ( FALSE );
	}

	InitRastPort( &rp );

	SetFont( &rp, table->Font );

	for( column = 0; column < TABLE_COLUMNS; column++ )
	{
		lengths[column] = strlen( cells[column] );
		bytes += lengths[column] + 1;

		widths[column] = TextLength( &rp, cells[column], lengths[column] ) + CELL_PADDING * 2 + 1;
		widths[column] = MAX( table->Columns[column], widths[column] );
		width += widths[column];
	}

	if ( width > INT16_MAX )
	{
		return ( FALSE );
	}

	row = AllocVecTags( bytes, TAG_END );

	if ( ! row )
	{
		return ( FALSE );
	}

	row->Next = NULL;
	text = row->Text;

	for( column = 0; column < TABLE_COLUMNS; column++ )
	{
		row->Cells[column] = text;
		table->Columns[column] = widths[column];

		memcpy( text, cells[column], lengths[column] + 1 );

		text += lengths[column] + 1;
	}

	if ( table->Last )
	{
		table->Last->Next = row;
	}
	else
	{
		table->First = row;
	}

	table->Last = row;
	image->Width = width;
	image->Height += table->RowHeight;

	return ( TRUE );
}

// --
