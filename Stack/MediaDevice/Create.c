
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "MediaDevice.h"
#include "Drivers/Audio/inc/AudioDevice.h"
#include "Drivers/Video/inc/VideoDevice.h"

// --

SEC_RODATA static const U16 AudioCommands[] =
{
	NSCMD_DEVICEQUERY,
	CMD_READ,
	CMD_WRITE,
	AUDIO_CMD_QUERY,
	AUDIO_CMD_SET_RATE,
	AUDIO_CMD_GET_GAIN,
	AUDIO_CMD_SET_GAIN,
	AUDIO_CMD_CHECK_RATE,
	AUDIO_CMD_QUEUE_WRITE,
	0
};

SEC_RODATA static const U16 VideoCommands[] =
{
	NSCMD_DEVICEQUERY,
	CMD_READ,
	VIDEO_CMD_QUERY,
	VIDEO_CMD_GET_MODE,
	VIDEO_CMD_SET_MODE,
	VIDEO_CMD_STOP,
	0
};

// --

SEC_CODE static U32
Obtain( struct Interface *Self )
{
	return ( ++Self->Data.RefCount );
}

// --

SEC_CODE static U32
Release( struct Interface *Self )
{
	return ( --Self->Data.RefCount );
}

// --

SEC_CODE static PTR
Expunge( struct DeviceManagerInterface *Self UNUSED )
{
	return ( NULL );
}

// --

SEC_RODATA static const PTR ManagerVectors[] =
{
	(PTR) Obtain,
	(PTR) Release,
	NULL,
	NULL,
	(PTR) MediaDevice_Open,
	(PTR) MediaDevice_Close,
	(PTR) Expunge,
	NULL,
	(PTR) MediaDevice_BeginIO,
	(PTR) MediaDevice_AbortIO,
	(PTR) -1
};

SEC_RODATA static const struct TagItem ManagerTags[] =
{
	{ MIT_Name, (Tag) "__device" },
	{ MIT_VectorTable, (Tag) ManagerVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

SEC_RODATA static const PTR BackendVectors[] =
{
	(PTR) Obtain,
	(PTR) Release,
	NULL,
	NULL,
	(PTR) MediaDevice_Register,
	(PTR) -1
};

SEC_RODATA static const struct TagItem BackendTags[] =
{
	{ MIT_Name, (Tag) "media" },
	{ MIT_VectorTable, (Tag) BackendVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

SEC_RODATA static const PTR Interfaces[] =
{
	(PTR) ManagerTags,
	(PTR) BackendTags,
	NULL
};

// --

SEC_CODE static struct USB2MediaDevice *
Alloc( struct ExecIFace *IExec, STR name, STR id, U16 version, U16 revision, const U16 *commands )
{
	struct USB2MediaDevice *base;

	base = (PTR) IExec->CreateLibraryTags(
		CLT_NoLegacyIFace, TRUE,
		CLT_Interfaces, Interfaces,
		CLT_DataSize, sizeof( *base ),
		TAG_END );

	if ( ! base )
	{
		return ( NULL );
	}

	base->Library.lib_Node.ln_Type = NT_DEVICE;
	base->Library.lib_Node.ln_Name = name;
	base->Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	base->Library.lib_Version = version;
	base->Library.lib_Revision = revision;
	base->Library.lib_IdString = id;
	base->IExec = IExec;
	base->Commands = commands;

	IExec->InitSemaphore( & base->Lock );

	return ( base );
}

// --

SEC_CODE S32
MediaDevice_Create( struct USBBase *usbbase )
{
	struct USB2MediaDevice *audio;
	struct USB2MediaDevice *video;
	struct ExecIFace *IExec;

	IExec = usbbase->usb_IExec;

	audio = Alloc( IExec, AUDIO_DEVICE_NAME, AUDIO_DEVICE_ID, AUDIO_DEVICE_VERSION, AUDIO_DEVICE_REVISION, AudioCommands );

	if ( ! audio )
	{
		return ( FALSE );
	}

	video = Alloc( IExec, VIDEO_DEVICE_NAME, VIDEO_DEVICE_ID, VIDEO_DEVICE_VERSION, VIDEO_DEVICE_REVISION, VideoCommands );

	if ( ! video )
	{
		IExec->DeleteLibrary( (PTR) audio );

		return ( FALSE );
	}

	usbbase->usb_Library.lib_OpenCnt++;

	IExec->AddDevice( (PTR) audio );

	IExec->AddDevice( (PTR) video );

	IExec->DebugPrintF( "USB2: " AUDIO_DEVICE_NAME " and " VIDEO_DEVICE_NAME " ready\n" );

	return ( TRUE );
}

// --
