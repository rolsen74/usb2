
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
WPA2_Process( struct USB2WPA2IFace *self __attribute__((unused)), struct USB2WPA2Session *handle,
	const U8 *frame, U32 length, struct USB2WPA2Reply *reply )
{
	struct WirelessKeySession *session;
	struct USB2WPA3Reply result;

	S32 status;

	session = (struct WirelessKeySession *) handle;

	memset( reply, 0, sizeof( *reply ) );

	if ( session->SAE )
	{
		return ( 0 );
	}

	status = Process_Key( session, frame, length, & result );

	if ( status > 0 )
	{
		reply->Length = result.Length;
		reply->Flags = result.Flags;
		reply->GroupIndex = result.GroupIndex;

		memcpy( reply->Frame, result.Frame, result.Length );

		memcpy( reply->PairwiseKey, result.PairwiseKey, sizeof( reply->PairwiseKey ) );

		memcpy( reply->GroupKey, result.GroupKey, sizeof( reply->GroupKey ) );

		memcpy( reply->GroupRSC, result.GroupRSC, sizeof( reply->GroupRSC ) );
	}

	Clear_Secret( & result, sizeof( result ) );

	return ( status );
}

// --
