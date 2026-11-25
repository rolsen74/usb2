/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"
#include <interfaces/usb2media.h>

// --

static inline BOOL
PublishMediaDevice( struct Library *backend )
{
	struct USB2MediaIFace *media;
	struct IORequest request;

	BOOL registered;

	request = (struct IORequest) { 0 };
	request.io_Message.mn_Length = sizeof( request );
	registered = FALSE;

	if ( OpenDevice( backend->lib_Node.ln_Name, USB2_MEDIA_QUERY_UNIT, & request, 0 ) )
	{
		// Older stacks create the public device when the class driver attaches.
		AddDevice( (struct Device *) backend );

		return ( TRUE );
	}

	media = (PTR) GetInterface( (PTR) request.io_Device, USB2_MEDIA_INTERFACE, USB2_MEDIA_INTERFACE_VERSION, NULL );

	if ( ! media )
	{
		media = (PTR) GetInterface( (PTR) request.io_Device, "media", 1, NULL );
	}

	if ( media )
	{
		registered = media->RegisterBackend( (struct Device *) backend );

		DropInterface( (PTR) media );
	}

	CloseDevice( & request );

	return ( registered );
}

// --

SEC_RODATA static const PTR Audio_Manager_Vectors[] =
{
	(PTR) Audio_Manager_Obtain,
	(PTR) Audio_Manager_Release,
	(PTR) NULL,
	(PTR) NULL,
	(PTR) Audio_Manager_Open,
	(PTR) Audio_Manager_Close,
	(PTR) Audio_Manager_Expunge,
	(PTR) NULL,
	(PTR) Audio_Manager_BeginIO,
	(PTR) Audio_Manager_AbortIO,
	(PTR) -1
};

SEC_RODATA static const struct TagItem Audio_Manager_Tags[] =
{
	{ MIT_Name, (Tag) "__device" },
	{ MIT_VectorTable, (Tag) Audio_Manager_Vectors },
	{ MIT_Version, (Tag) 1 },
	{ TAG_END, (Tag) 0 }
};

SEC_RODATA static const PTR Audio_Interfaces[] =
{
	(PTR) Audio_Manager_Tags,
	(PTR) NULL
};

SEC_RODATA static const struct TagItem Audio_CreateTags[] =
{
	{ CLT_DataSize, (Tag) sizeof( struct AudioBase ) },
	{ CLT_InitFunc, (Tag) Audio_Manager_Init },
	{ CLT_Interfaces, (Tag) Audio_Interfaces },
	{ TAG_END, (Tag) 0 }
};

// --

SEC_CODE S32 Audio_CreateDevice( struct AudioContext *ctx )
{
struct AudioBase *base;

	ObtainSemaphore( & ctx->Lock );

	if ( ctx->Device )
	{
		ReleaseSemaphore( & ctx->Lock );

		return ( TRUE );
	}

	base = (PTR) CreateLibrary( (PTR) Audio_CreateTags );

	if ( ! base )
	{
		ReleaseSemaphore( & ctx->Lock );

		return ( FALSE );
	}

	base->Context = ctx;
	InitSemaphore( & base->Semaphore );

	if ( ! PublishMediaDevice( & base->Library ) )
	{
		DeleteLibrary( (PTR) base );

		ReleaseSemaphore( & ctx->Lock );

		return ( FALSE );
	}

	ctx->Device = base;

	ReleaseSemaphore( & ctx->Lock );

	return ( TRUE );
}

// --
