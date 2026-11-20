
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
Wireless_PromptPassword( struct Wireless *wireless, U32 *action )
{
	struct USB2WiFiPrompt prompt;
	struct IOSana2Req *event;
	struct AutoWiFi *wifi;
	struct MsgPort *port;

	U32 mask;
	U32 signals;
	S32 result;
	S32 started;

	port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! port )
	{
		fprintf( stderr, "Could not allocate the Wi-Fi requester reply port.\n" );

		return ( USB2WIFI_ERROR );
	}

	wifi = wireless->Auto;
	event = wireless->Receive;
	result = USB2WIFI_ERROR;

	memset( & prompt, 0, sizeof( prompt ) );

	prompt.Death.dm_Msg.mn_ReplyPort = port;
	prompt.Security = wifi->Profile.Security;
	prompt.Retry = wifi->Retry != 0;

	memcpy( prompt.SSID, wifi->Profile.SSID, sizeof( prompt.SSID ) );

	event->ios2_Req.io_Command = S2_ONEVENT;
	event->ios2_Req.io_Flags = 0;
	event->ios2_WireError = S2EVENT_HARDWARE;

	SendIO( & event->ios2_Req );

	wireless->Command->ios2_Data = & prompt;
	wireless->Command->ios2_DataLength = sizeof( prompt );

	started = Wireless_Command( wireless, USB2WIRELESS_CMD_PROMPT );

	wireless->Command->ios2_Data = NULL;
	wireless->Command->ios2_DataLength = 0;

	if ( started )
	{
		mask = ( 1UL << port->mp_SigBit ) | ( 1UL << wireless->ReceivePort->mp_SigBit ) | SIGBREAKF_CTRL_C;

		for( ;; )
		{
			if ( GetMsg( port ) )
			{
				if ( ! CheckIO( & event->ios2_Req ) )
				{
					result = prompt.Result;
				}
				else
				{
					fprintf( stderr, "Wi-Fi adapter stopped while waiting for a password.\n" );
				}

				goto done;
			}

			if ( CheckIO( & event->ios2_Req ) )
			{
				fprintf( stderr, "Wi-Fi adapter stopped while waiting for a password.\n" );
				break;
			}

			signals = Wait( mask );

			if ( signals & SIGBREAKF_CTRL_C )
			{
				result = USB2WIFI_MISSING;
				break;
			}
		}

		wifi->Interface->CancelPrompt( & prompt );

		WaitPort( port );

		GetMsg( port );
	}
	else if ( wireless->Command->ios2_Req.io_Error == IOERR_NOCMD )
	{
		fprintf( stderr, "Update %s for device-owned Wi-Fi password requests.\n", wireless->DeviceName );
	}

done:

	if ( ! CheckIO( & event->ios2_Req ) )
	{
		AbortIO( & event->ios2_Req );
	}

	WaitIO( & event->ios2_Req );

	if ( result == USB2WIFI_OK )
	{
		memcpy( wifi->Profile.Password, prompt.Password, sizeof( wifi->Profile.Password ) );

		*action = prompt.PasswordAction;
	}
	else if ( result == USB2WIFI_ERROR && started )
	{
		fprintf( stderr, "The Wi-Fi password window could not be used.\n" );
	}

	Clear_Secret( prompt.Password, sizeof( prompt.Password ) );

	FreeSysObject( ASOT_PORT, port );

	return ( result );
}

// --
