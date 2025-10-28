
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

SEC_CODE struct TagItem *__Misc_NextTagItem( struct USBBase *usbbase, struct TagItem **tag )
{
struct UtilityIFace *IUtility;
struct TagItem *tagitem;

	IUtility = usbbase->usb_IUtility;
	tagitem = IUtility->NextTagItem( tag );

	return( tagitem );
}

// --
