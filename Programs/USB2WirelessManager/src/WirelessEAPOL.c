
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

static ULONG
Copy_Packet( struct Hook *hook __attribute__((unused)), struct IOSana2Req *io __attribute__((unused)),
	struct SANA2CopyHookMsg *copy )
{
	if ( ( copy->schm_Size > WIRELESS_FRAME_SIZE ) ||
		( ( copy->schm_Method != S2_CopyToBuff ) && ( copy->schm_Method != S2_CopyFromBuff ) ) )
	{
		return ( FALSE );
	}

	CopyMem( copy->schm_From, copy->schm_To, copy->schm_Size );

	return ( TRUE );
}

// --

S32
Wireless_Set_Copy_Hook( struct Wireless *wireless )
{
	static Tag methods[] = { S2_CopyToBuff, S2_CopyFromBuff, TAG_END };

	S32 result;

	wireless->CopyHook.s2h_Hook.h_Entry = (HOOKFUNC) Copy_Packet;
	wireless->CopyHook.s2h_Methods = methods;
	wireless->Command->ios2_Data = & wireless->CopyHook;
	wireless->Command->ios2_DataLength = sizeof( wireless->CopyHook );

	result = Wireless_Command( wireless, S2_SANA2HOOK );

	wireless->Command->ios2_Data = NULL;

	return ( result );
}

// --

S32
Wireless_Send_EAPOL( struct Wireless *wireless, const U8 *frame, U32 length, U32 flags )
{
	struct IOSana2Req *io;

	S32 result;

	io = wireless->Command;
	io->ios2_Req.io_Flags = flags & USB2WPA2_CLEAR_REPLY ? USB2RTL8188IOF_CLEAR_EAPOL : 0;
	io->ios2_PacketType = WIRELESS_EAPOL;
	io->ios2_DataLength = length;
	io->ios2_Data = (void *) frame;

	memcpy( io->ios2_DstAddr, wireless->AP.BSSID, WIRELESS_ADDR_SIZE );

	result = Wireless_Command( wireless, CMD_WRITE );

	io->ios2_Req.io_Flags = 0;
	io->ios2_Data = NULL;

	return ( result );
}
