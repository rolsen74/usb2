
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "usb2_all.h"

// --

SEC_CODE static S32 __Clr_Feature( 
		struct USB2_SetupData *sd, 
UNUSED	struct USB2_HCDNode *hn, 
UNUSED	struct USBBase *usbbase )
{
//U16 wLength;
U16 wValue;
U16 wIndex;
U16 port;
S32 err;

	err		= USB2Err_Host_Stall;
	wValue	= LE_SWAP16( sd->Value );
	wIndex	= LE_SWAP16( sd->Index );
//	wLength	= LE_SWAP16( sd->Length );
	port	= ( wIndex & 0x00ff );

	/**/ if	( sd->RequestType == ( REQTYPE_Write | REQTYPE_Class | REQTYPE_Device ))
	{
		if ( ( wValue == HUB_CLR_OVER_CURRENT ) && ( hn->HCD_Functions.Hub_Clr_OverCurrent_Chg ) )
		{
			hn->HCD_Functions.Hub_Clr_OverCurrent_Chg( hn );

			err = USB2Err_NoError;
		}
	}
	else if ( sd->RequestType == ( REQTYPE_Write | REQTYPE_Class | REQTYPE_Other ))
	{
		if ( ( ! port ) || ( port > hn->hn_HUB_NumPorts ) )
		{
			return ( USB2Err_Host_Stall );
		}

		// Clear Port Status

		USBDEBUG( "__Clr_Feature : Clear Port Feature on port #%lu", port );

		switch( wValue )
		{
			case PORT_ENABLE:
			{
				if ( hn->HCD_Functions.Port_Clr_Enable )
				{
					err = hn->HCD_Functions.Port_Clr_Enable( hn, port );
				}

				break;
			}

			case PORT_SUSPEND:
			{
				if ( ( ( hn->hn_IsOHCI ) || ( hn->hn_HCDType == HCDTYPE_UHCI ) ) && ( hn->HCD_Functions.Port_Clr_Suspend ) )
				{
					err = hn->HCD_Functions.Port_Clr_Suspend( hn, port );
				}

				break;
			}

			case PORT_POWER:
			{
				if ( ( hn->hn_IsOHCI ) && ( hn->HCD_Functions.Port_Clr_Power ) )
				{
					err = hn->HCD_Functions.Port_Clr_Power( hn, port );
				}

				break;
			}

			case PORT_CLR_CONNECTION:
			{
//				err = PORT_CLEAR_CONNECTION_CHANGE( port );
				if ( hn->HCD_Functions.Port_Clr_Connection_Chg )
				{
					USBDEBUG( "__Clr_Feature : PORT_CLR_CONNECTION : Port #%ld", port );

					err = hn->HCD_Functions.Port_Clr_Connection_Chg( hn, port );

					if ( ( err == USB2Err_NoError ) && ( port < sizeof( hn->hn_RescanPort ) ) )
					{
						hn->hn_RescanPort[port] = FALSE;
					}
				}
				#ifdef DO_DEBUG
				else
				{
					USBERROR( "__Clr_Feature : PORT_CLR_CONNECTION : Not Implemented in HCD" );
					err = -1;
				}
				#endif
				break;
			}

			case PORT_CLR_BH_RESET:
			case PORT_CLR_LINK_STATE:
			case PORT_CLR_CONFIG_ERROR:
			{
				if ( hn->HCD_Functions.Port_Clr_Super_Change )
				{
					err = hn->HCD_Functions.Port_Clr_Super_Change( hn, port, wValue );
				}

				break;
			}

			case PORT_CLR_ENABLE:
			{
//				err = PORT_CLEAR_ENABLE_CHANGE( port );
				if ( hn->HCD_Functions.Port_Clr_Enable_Chg )
				{
					USBDEBUG( "__Clr_Feature : PORT_CLR_ENABLE : Port #%ld", port );

					err = hn->HCD_Functions.Port_Clr_Enable_Chg( hn, port );
				}
				#ifdef DO_DEBUG
				else
				{
					USBERROR( "__Clr_Feature : PORT_CLR_ENABLE : Not Implemented in HCD" );
					err = -1;
				}
				#endif
				break;
			}

			case PORT_CLR_SUSPEND:
			{
//				err = PORT_CLEAR_SUSPEND_CHANGE( port );
				if ( hn->HCD_Functions.Port_Clr_Suspend_Chg )
				{
					USBDEBUG( "__Clr_Feature : PORT_CLR_SUSPEND : Port #%ld", port );

					err = hn->HCD_Functions.Port_Clr_Suspend_Chg( hn, port );
				}
				#ifdef DO_DEBUG
				else
				{
					USBERROR( "__Clr_Feature : PORT_CLR_SUSPEND : Not Implemented in HCD" );
					err = -1;
				}
				#endif
				break;
			}

			case PORT_CLR_OVER_CURRENT:
			{
//				err = PORT_CLEAR_OVERCURRENT_CHANGE( port );
				if ( hn->HCD_Functions.Port_Clr_OverCurrent_Chg )
				{
					USBDEBUG( "__Clr_Feature : PORT_CLR_OVER_CURRENT : Port #%ld", port );

					err = hn->HCD_Functions.Port_Clr_OverCurrent_Chg( hn, port );
				}
				#ifdef DO_DEBUG
				else
				{
					USBERROR( "__Clr_Feature : PORT_CLR_OVER_CURRENT : Not Implemented in HCD" );
					err = -1;
				}
				#endif
				break;
			}

			case PORT_CLR_RESET:
			{
//				err = PORT_CLEAR_RESET_CHANGE( port );
				if ( hn->HCD_Functions.Port_Clr_Reset_Chg )
				{
					USBDEBUG( "__Set_Feature : PORT_CLR_RESET : Port #%ld", port );

					err = hn->HCD_Functions.Port_Clr_Reset_Chg( hn, port );
				}
				#ifdef DO_DEBUG
				else
				{
					USBERROR( "__Set_Feature : PORT_CLR_RESET : Not Implemented in HCD" );
					err = -1;
				}
				#endif
				break;
			}

			default:
			{
				USBDEBUG( "__Clr_Feature : Unknown Status (%lu)", wValue );
				break;
			}
		}
	}

	return( err );
}

// --
