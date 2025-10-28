
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "__Ctrl_Write_Clr_Feature.c"
#include "__Ctrl_Write_Set_Config.c"
#include "__Ctrl_Write_Set_Feature.c"

// --

SEC_CODE static void __Control_Write( struct USBBase *usbbase, struct USB2_HCDNode *hn, struct RealRequest *ioreq )
{
struct USB2_SetupData *sd;
S32 err;

	sd = ioreq->req_Public.io_SetupData;

	if ( ! sd )
	{
		USBDEBUG( "Root HUB : Write : No SetupData buffer given" );

		err = USB2Err_Host_Stall;
		return;
	}
	else
	{
		switch( sd->RequestCode )
		{
			case REQCODE_Set_Feature:
			{
				USBDEBUG( "Root HUB : Write : Set Feature" );

				err = __Set_Feature( sd, hn, usbbase );
				break;
			}

			case REQCODE_Clear_Feature:
			{
				USBDEBUG( "Root HUB : Write : Clear Feature" );

				err = __Clr_Feature( sd, hn, usbbase );
				break;
			}

			case REQCODE_Set_Configuration:
			{
				USBDEBUG( "Root HUB : Write : Set Configuration Feature" );

				err = __Set_Config( sd, hn, usbbase );
				break;
			}

			default:
			{
				USBDEBUG( "Root HUB : Write : Unknown Request (%lu)", (U32) sd->RequestCode );

				err = USB2Err_Host_Stall;
				break;
			}
		}
	}

	ioreq->req_Public.io_Error = err;
}

// --
