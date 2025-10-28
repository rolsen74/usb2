
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

SEC_CODE U32 __Misc_GetTagValue( struct USBBase *usbbase, U32 tagid, U32 defvalue, struct TagItem *taglist )
{
struct UtilityIFace *IUtility;
U32 retval;

	IUtility = usbbase->usb_IUtility;
	retval = (U32) IUtility->GetTagData( tagid, defvalue, taglist );

//	USBDEBUG( 11, "GetTagValue( %08lx [%08lx] ) -> %08lx", tagid, defvalue, retval );

	return( retval );
}

// --
