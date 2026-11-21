
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/SmartCard.h"

// --

S32
SmartCard_Parse( struct USB2_DriverMessage *msg, struct USB2SmartCardInfo *info )
{
	struct USB2IFace *usb;
	const U8 *desc;

	U32 remaining;
	U32 length;
	U32 maximum;
	S32 selected;

	usb = msg->IUSB2;
	desc = (PTR) msg->ConfigDescriptors;
	selected = FALSE;

	if ( ( ! desc ) || ( ! msg->Function ) ||
		( usb->USB2_Attr_Get( msg->Function->FKT_ID, USB2Tag_Attr_ConfigSize,
			& remaining, sizeof( remaining ) ) != sizeof( remaining ) ) )
	{
		return ( FALSE );
	}

	while( remaining >= 2 )
	{
		length = desc[0];

		if ( ( length < 2 ) || ( length > remaining ) )
		{
			break;
		}

		if ( desc[1] == DSCTYPE_Interface )
		{
			selected = ( length >= 9 ) && ( desc[2] == msg->InterfaceDescriptor->InterfaceNumber ) &&
				( desc[3] == msg->InterfaceDescriptor->AlternateSetting );
		}
		else if ( ( selected ) && ( desc[1] == 0x21 ) && ( length >= 54 ) )
		{
			maximum = CCID_Read32( desc + 44 );

			if ( ( maximum < CCID_HEADER ) || ( maximum > CCID_MAX_MESSAGE ) )
			{
				break;
			}

			info->MaxMessage = maximum;

			info->Features = CCID_Read32( desc + 40 );

			info->Vendor = msg->VendorID;
			info->Product = msg->DeviceID;
			info->Slots = (UWORD) desc[4] + 1;

			return ( TRUE );
		}

		desc += length;
		remaining -= length;
	}

	return ( FALSE );
}

// --
