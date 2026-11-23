
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include <strings.h>

// --

static void
Read_Adapter( const struct IORequest *request, struct USB2EthAdapterInfo *info, ULONG *registration )
{
	struct USB2EthIFace *ethernet;
	UWORD revision;

	memset( info, 0, sizeof( *info ) );

	*registration = 0;

	revision = request->io_Device->dd_Library.lib_Revision;

	if ( revision >= USB2ETH_ADAPTER_REVISION )
	{
		ethernet = (APTR) GetInterface( (APTR) request->io_Device, "main", 1, NULL );

		if ( ethernet )
		{
			if ( revision >= USB2ETH_CLASS_REVISION )
			{
				ethernet->GetAdapterInfo( request, info );
			}
			else
			{
				info->Type = ethernet->GetAdapter( request, 0 );
			}

			if ( revision >= USB2ETH_REGISTRATION_REVISION )
			{
				*registration = ethernet->GetRegistrationID( request, 0 );

				if ( ! *registration )
				{
					info->Type = USB2ETH_ADAPTER_UNKNOWN;
				}
			}

			DropInterface( (APTR) ethernet );
		}
	}
}

// --

ULONG
Wireless_ResolveAdapter( const char *selector, ULONG number, ULONG *registration )
{
	struct IOSana2Req request;
	struct USB2EthAdapterInfo info;

	*registration = 0;

	memset( & request, 0, sizeof( request ) );

	request.ios2_Req.io_Message.mn_Length = sizeof( request );

	if ( OpenDevice( WIRELESS_DEVICE, number, & request.ios2_Req, 0 ) )
	{
		return ( USB2ETH_ADAPTER_UNKNOWN );
	}

	Read_Adapter( & request.ios2_Req, & info, registration );

	CloseDevice( & request.ios2_Req );

	if ( ( ! selector ) || ( ! strcasecmp( selector, WIRELESS_DEVICE ) ) ||
		( ( info.Title[0] ) && ( ! strcasecmp( selector, info.Title ) ) ) ||
		( ( info.Driver[0] ) && ( ! strcasecmp( selector, info.Driver ) ) ) ||
		( ( info.Alias[0] ) && ( ! strcasecmp( selector, info.Alias ) ) ) )
	{
		return ( info.Type );
	}

	return ( USB2ETH_ADAPTER_UNKNOWN );
}

// --

S32
Wireless_Open( struct Wireless *wireless )
{
	struct USB2EthAdapterInfo info;

	ULONG registration;
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

	error = OpenDevice( WIRELESS_DEVICE, wireless->UnitNumber, & wireless->Command->ios2_Req, 0 );

	if ( error )
	{
		fprintf( stderr, "Could not open %s unit %lu (%ld).\n", WIRELESS_DEVICE,
			(unsigned long) wireless->UnitNumber, (long) error );

		return ( FALSE );
	}

	wireless->DeviceOpen = TRUE;

	Read_Adapter( & wireless->Command->ios2_Req, & info, &registration );

	if ( ( info.Type == USB2ETH_ADAPTER_UNKNOWN ) || ( info.Type != wireless->AdapterType ) ||
		( ( wireless->Registration ) && ( wireless->Registration != registration ) ) )
	{
		fprintf( stderr, "%s unit %lu has no matching Wi-Fi adapter.\n",
			WIRELESS_DEVICE, (unsigned long) wireless->UnitNumber );

		return ( FALSE );
	}

	wireless->DeviceName = WIRELESS_DEVICE;
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
Wireless_Reset_Options( struct Wireless *wireless )
{
	static const U8 empty[WIRELESS_ADDR_SIZE] = { 0 };

	if ( wireless->SetOptions )
	{
		Wireless_Set_Options( wireless, 0, empty, 0, S2ENC_NONE, FALSE );

		wireless->SetOptions = FALSE;
	}
}

// --

void
Wireless_Close( struct Wireless *wireless )
{
	if ( wireless->DeviceOpen )
	{
		Wireless_Reset_Options( wireless );

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
		io->ios2_Req.io_Flags = command == S2_CONFIGINTERFACE ? USB2WIRELESS_IOF_MANAGER_CONFIG : 0;
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
	struct TagItem tags[8];

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
	tags[6].ti_Tag = ( wireless->UseSAE && encryption == S2ENC_CCMP ) ? USB2WIRELESSINFO_SAE : TAG_IGNORE;
	tags[6].ti_Data = TRUE;
	tags[7].ti_Tag = TAG_END;
	tags[7].ti_Data = 0;
	wireless->Command->ios2_Data = tags;

	result = Wireless_Command( wireless, S2_SETOPTIONS );

	wireless->SetOptions |= result;
	wireless->Command->ios2_Data = NULL;

	return ( result );
}

// --

S32
Wireless_Set_Key( struct Wireless *wireless, U32 index, const U8 *key, const U8 *rsc, U32 cipher, U32 length )
{
	struct IOSana2Req *io;

	S32 result;

	io = wireless->Command;
	io->ios2_WireError = index;
	io->ios2_PacketType = cipher;
	io->ios2_Data = (void *) key;
	io->ios2_DataLength = length;
	io->ios2_StatData = (void *) rsc;

	result = Wireless_Command( wireless, S2_SETKEY );

	io->ios2_Data = NULL;
	io->ios2_StatData = NULL;

	return ( result );
}
