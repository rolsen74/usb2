
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "usb2_all.h"

// --

SEC_CODE struct USB2_Descriptor *__Desc_Next_Desc( struct USBBase *usbbase UNUSED, struct USB2_Descriptor *desc )
{
struct USB2_Descriptor *dsc;
U32 pos;
U8 *data;

	dsc = NULL;

	if (( desc ) 
	&&	( desc->Length ) 
	&&	( desc->DescriptorType ))
	{
		pos		= desc->Length;
		data	= (PTR) desc;
		desc	= (PTR) & data[pos];

		if (( desc->Length ) 
		&&	( desc->DescriptorType ))
		{
			dsc = desc;
		}
	}

	return( dsc );
}

// --
