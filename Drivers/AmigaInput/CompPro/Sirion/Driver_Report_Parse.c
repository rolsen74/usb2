
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --

S32 Driver_Report_Parse( struct USB_Driver *in )
{

U32 field_count;
S32 retval;
U32 value;
U16 usage;
U32 size;
U32 pos;
U8 *desc;
U8 report_count;
U8 report_size;
U8 usage_page;
U8 report_id;
U8 item_size;
U8 item_type;
U8 item_tag;
U8 len;
U8 b;
U8 i;

	retval = FALSE;

	desc = in->Report_Buffer;
	size = in->Report_Size;

	field_count		= 0;
	report_count	= 0;
	report_size		= 0;
	report_id		= 0;
	usage_page		= 0;
	usage			= 0;
	pos				= 0;

	while( pos < size )
	{
		b = desc[ pos++ ];
		
		item_size	= ( b >> 0 ) & 0x03;
		item_type	= ( b >> 2 ) & 0x03;
		item_tag	= ( b >> 4 ) & 0x0F;

		if ( b == 0xFE ) 
		{
			// Skip long item
			len  = desc[ pos ];
			pos += len + 2;
			continue;
		}

		value = 0;

		for( i=0 ; i < item_size ; i++ ) 
		{
			value |= ( (U32) desc[ pos++ ] ) << ( i * 8 );
		}

		// Main items
		if ( item_type == 0x0 )
		{
			switch( item_tag ) 
			{
				case 8:  // Input
				case 9:  // Output
				case 11: // Feature
				{
					if ( field_count < MAX_REPORT_FIELDS )
					{
						in->Report_Fields[field_count].report_id	= report_id;
						in->Report_Fields[field_count].usage_page	= usage_page;
						in->Report_Fields[field_count].usage		= usage;
						in->Report_Fields[field_count].report_size	= report_size;
						in->Report_Fields[field_count].report_count = report_count;
						in->Report_Fields[field_count].type			= ( item_tag == 8 ) ? 0 : (( item_tag == 9 ) ? 1 : 2 );
						in->Report_Fields[field_count].flags		= value;
						field_count++;
					}
					break;
				}
			}
		}

		// Global items
		else if ( item_type == 0x1 )
		{
			switch( item_tag )
			{
				case 0: usage_page		= value; break;
				case 7: report_size		= value; break;
				case 8: report_id		= value; break;
				case 9: report_count	= value; break;
			}
		}

		// Local items
		else if ( item_type == 0x2 ) 
		{
			switch( item_tag )
			{
				case 0: usage = value; break;
			}
		}
	}

	#if 1

	const char *type_str[] = { "Input", "Output", "Feature" };

	for( U32 i=0 ; i < field_count; i++ )
	{
		struct HID_ReportField *f = & in->Report_Fields[i];

		DebugPrintF(
			"Field %2lu: ReportID %2u, Type: %-7s, UsagePage: 0x%02X, Usage: 0x%04X, Size: %2u bits x %2u\n",
			i,
			f->report_id,
			type_str[f->type],
			f->usage_page,
			f->usage,
			f->report_size,
			f->report_count
		);
	}

	#endif

	retval = TRUE;

bailout:

	return( retval );
}
