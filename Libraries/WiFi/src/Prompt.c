
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

#include <dos/dostags.h>

// --

static S32
PromptProcess( void )
{
	struct USB2WiFiPrompt *prompt;
	struct USB2WiFiProfile profile;
	struct Task *task;

	S32 cancelled;

	task = FindTask( NULL );
	prompt = task->tc_UserData;

	ObtainSemaphore( & WiFiLock );

	cancelled = prompt->Cancelled;
	prompt->Worker = task;

	ReleaseSemaphore( & WiFiLock );

	if ( ! cancelled )
	{
		memset( & profile, 0, sizeof( profile ) );

		profile.Security = prompt->Security;

		memcpy( profile.SSID, prompt->SSID, sizeof( profile.SSID ) );

		prompt->Result = WiFi_Window( & profile, 0, 0, FALSE, prompt->Retry, & prompt->PasswordAction );

		if ( prompt->Result == USB2WIFI_OK )
		{
			memcpy( prompt->Password, profile.Password, sizeof( prompt->Password ) );
		}

		{
			volatile char *p;
			U32 i;

			p = profile.Password;

			for( i = 0; i < sizeof( profile.Password ); i++ )
			{
				p[i] = 0;
			}
		}
	}

	ObtainSemaphore( & WiFiLock );

	prompt->Worker = NULL;

	ReleaseSemaphore( & WiFiLock );

	// DOS sends Death only after this entry point has returned.
	return ( RETURN_OK );
}

// --

S32
WiFi_StartPrompt( struct USB2WiFiIFace *self __attribute__((unused)), struct USB2WiFiPrompt *prompt )
{
	if ( ( ! prompt ) || ( ! prompt->Death.dm_Msg.mn_ReplyPort ) ||
		( ! prompt->SSID[0] ) || ( ! memchr( prompt->SSID, 0, sizeof( prompt->SSID ) ) ) ||
		( prompt->Security < USB2WIFI_WPA ) || ( prompt->Security > USB2WIFI_WPA3 ) )
	{
		return ( FALSE );
	}

	prompt->Result = USB2WIFI_MISSING;
	prompt->Worker = NULL;
	prompt->Cancelled = FALSE;
	prompt->PasswordAction = USB2WIFI_KEEP_PASSWORD;
	prompt->Death.dm_Msg.mn_Node.ln_Type = NT_MESSAGE;
	prompt->Death.dm_Msg.mn_Length = sizeof( prompt->Death );

	memset( prompt->Password, 0, sizeof( prompt->Password ) );

	return ( CreateNewProcTags( NP_Name, "USB2 Ethernet - Wi-Fi password",
		NP_Entry, PromptProcess, NP_UserData, prompt, NP_Child, FALSE, NP_StackSize, 32768,
		NP_NotifyOnDeathMessage, & prompt->Death, TAG_END ) != NULL );
}

// --

void
WiFi_CancelPrompt( struct USB2WiFiIFace *self __attribute__((unused)), struct USB2WiFiPrompt *prompt )
{
	ObtainSemaphore( & WiFiLock );

	prompt->Cancelled = TRUE;

	if ( prompt->Worker )
	{
		Signal( prompt->Worker, SIGBREAKF_CTRL_C );
	}

	ReleaseSemaphore( & WiFiLock );
}

// --
