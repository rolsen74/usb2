
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
Wireless_Open( struct Wireless *wireless )
{
	S32 error;

	wireless->CommandPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	wireless->ReceivePort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	wireless->TimerPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ( ! wireless->CommandPort ) || ( ! wireless->ReceivePort ) || ( ! wireless->TimerPort ) )
	{
		fprintf( stderr, "Could not allocate message ports.\n" );

		return ( FALSE );
	}

	wireless->Command = AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( *wireless->Command ), ASOIOR_ReplyPort, wireless->CommandPort, TAG_END );

	wireless->Receive = AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( *wireless->Receive ), ASOIOR_ReplyPort, wireless->ReceivePort, TAG_END );

	wireless->Timer = AllocSysObjectTags( ASOT_IOREQUEST,
		ASOIOR_Size, sizeof( *wireless->Timer ), ASOIOR_ReplyPort, wireless->TimerPort, TAG_END );

	if ( ( ! wireless->Command ) || ( ! wireless->Receive ) || ( ! wireless->Timer ) )
	{
		fprintf( stderr, "Could not allocate I/O requests.\n" );

		return ( FALSE );
	}

	error = OpenDevice( TIMERNAME, UNIT_MICROHZ, & wireless->Timer->Request, 0 );

	if ( error )
	{
		fprintf( stderr, "Could not open %s (%ld).\n", TIMERNAME, (long) error );

		return ( FALSE );
	}

	wireless->TimerOpen = TRUE;
	wireless->Command->ios2_BufferManagement = NULL;

	error = OpenDevice( WIRELESS_DEVICE_PATH, wireless->UnitNumber, & wireless->Command->ios2_Req, 0 );

	if ( error )
	{
		fprintf( stderr, "Could not open %s unit %lu (%ld).\n", WIRELESS_DEVICE_PATH,
			(unsigned long) wireless->UnitNumber, (long) error );

		return ( FALSE );
	}

	wireless->DeviceOpen = TRUE;
	wireless->Receive->ios2_Req.io_Device = wireless->Command->ios2_Req.io_Device;
	wireless->Receive->ios2_Req.io_Unit = wireless->Command->ios2_Req.io_Unit;
	wireless->Receive->ios2_BufferManagement = wireless->Command->ios2_BufferManagement;

	if ( ! Wireless_Command( wireless, S2_GETSTATIONADDRESS ) )
	{
		return ( FALSE );
	}

	memcpy( wireless->Station, wireless->Command->ios2_SrcAddr, sizeof( wireless->Station ) );

	return ( TRUE );
}

// --

void
Wireless_Close( struct Wireless *wireless )
{
	static const U8 empty[WIRELESS_ADDR_SIZE] = { 0 };

	if ( wireless->DeviceOpen )
	{
		if ( wireless->SetOptions )
		{
			Wireless_Set_Options( wireless, 0, empty, 0, S2ENC_NONE, FALSE );
		}

		CloseDevice( & wireless->Command->ios2_Req );
	}

	if ( wireless->TimerOpen )
	{
		CloseDevice( & wireless->Timer->Request );
	}

	if ( wireless->Timer )
	{
		FreeSysObject( ASOT_IOREQUEST, wireless->Timer );
	}

	if ( wireless->Receive )
	{
		FreeSysObject( ASOT_IOREQUEST, wireless->Receive );
	}

	if ( wireless->Command )
	{
		FreeSysObject( ASOT_IOREQUEST, wireless->Command );
	}

	if ( wireless->TimerPort )
	{
		FreeSysObject( ASOT_PORT, wireless->TimerPort );
	}

	if ( wireless->ReceivePort )
	{
		FreeSysObject( ASOT_PORT, wireless->ReceivePort );
	}

	if ( wireless->CommandPort )
	{
		FreeSysObject( ASOT_PORT, wireless->CommandPort );
	}
}

// --

S32
Wireless_Command( struct Wireless *wireless, U16 command )
{
	struct IOSana2Req *io;

	io = wireless->Command;
	io->ios2_Req.io_Command = command;
	if ( command != CMD_WRITE )
	{
		io->ios2_Req.io_Flags = 0;
	}
	if ( command != S2_SETKEY )
	{
		io->ios2_WireError = 0;
	}

	DoIO( & io->ios2_Req );

	if ( ( command == S2_CONFIGINTERFACE ) && ( io->ios2_Req.io_Error == S2ERR_BAD_STATE ) &&
		( io->ios2_WireError == S2WERR_IS_CONFIGURED ) )
	{
		return ( TRUE );
	}

	if ( io->ios2_Req.io_Error )
	{
		fprintf( stderr, "SANA-II command 0x%04x failed (%d, wire error %lu).\n", (unsigned) command,
			(int) io->ios2_Req.io_Error, (unsigned long) io->ios2_WireError );

		return ( FALSE );
	}

	return ( TRUE );
}

// --

S32
Wireless_Set_Options( struct Wireless *wireless, U32 channel, const U8 *bssid, U16 aid,
	U32 encryption, S32 operational )
{
	struct TagItem tags[7];

	S32 result;

	tags[0].ti_Tag = channel ? S2INFO_Channel : TAG_IGNORE;
	tags[0].ti_Data = channel;
	tags[1].ti_Tag = S2INFO_BSSID;
	tags[1].ti_Data = (uintptr_t) bssid;
	tags[2].ti_Tag = S2INFO_AssocID;
	tags[2].ti_Data = aid;
	tags[3].ti_Tag = S2INFO_PortType;
	tags[3].ti_Data = S2PORT_MANAGED;
	tags[4].ti_Tag = S2INFO_Encryption;
	tags[4].ti_Data = encryption;
	tags[5].ti_Tag = S2INFO_OperState;
	tags[5].ti_Data = operational;
	tags[6].ti_Tag = TAG_END;
	tags[6].ti_Data = 0;
	wireless->Command->ios2_Data = tags;

	result = Wireless_Command( wireless, S2_SETOPTIONS );

	wireless->SetOptions |= result;
	wireless->Command->ios2_Data = NULL;

	return ( result );
}

// --

S32
Wireless_Set_Key( struct Wireless *wireless, U32 index, const U8 *key, const U8 *rsc )
{
	struct IOSana2Req *io;

	S32 result;

	io = wireless->Command;
	io->ios2_WireError = index;
	io->ios2_PacketType = S2ENC_CCMP;
	io->ios2_Data = (void *) key;
	io->ios2_DataLength = 16;
	io->ios2_StatData = (void *) rsc;

	result = Wireless_Command( wireless, S2_SETKEY );

	io->ios2_Data = NULL;
	io->ios2_StatData = NULL;

	return ( result );
}
