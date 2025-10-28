
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

SEC_CODE static S32 __Set_Config( 
		struct USB2_SetupData *sd, 
		struct USB2_HCDNode *hn, 
UNUSED	struct USBBase *usbbase )
{
U16 wLength;
U16 wValue;
U16 wIndex;
S32 err;

	err		= USB2Err_Host_Stall;
	wValue	= LE_SWAP16( sd->Value );
	wIndex	= LE_SWAP16( sd->Index );
	wLength	= LE_SWAP16( sd->Length );

	if (( wValue == HUB_Cfg_Nr ) && ( wIndex == 0 ) && ( wLength == 0 ))
	{
		USBDEBUG( "__Set_Config: Setting Config Nr #%ld", (S32) wValue );
		hn->hn_HUB_ConfigNr = wValue;
		err = USB2Err_NoError;
	}
	else
	{
		USBDEBUG( "__Set_Config: hmm unknown 11" );
		err = USB2Err_Host_Stall;
	}

	return( err );
}

// --
