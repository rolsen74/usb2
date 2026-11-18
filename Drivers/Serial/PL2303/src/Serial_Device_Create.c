/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

SEC_RODATA static const PTR Serial_Manager_Vectors[] =
{
	(PTR) Serial_Manager_Obtain,
	(PTR) Serial_Manager_Release,
	(PTR) NULL,
	(PTR) NULL,
	(PTR) Serial_Manager_Open,
	(PTR) Serial_Manager_Close,
	(PTR) Serial_Manager_Expunge,
	(PTR) NULL,
	(PTR) Serial_Manager_BeginIO,
	(PTR) Serial_Manager_AbortIO,
	(PTR) -1
};

SEC_RODATA static const struct TagItem Serial_Manager_Tags[] =
{
	{ MIT_Name, (Tag) "__device" },
	{ MIT_VectorTable, (Tag) Serial_Manager_Vectors },
	{ MIT_Version, (Tag) 1 },
	{ TAG_END, (Tag) 0 }
};

SEC_RODATA static const PTR Serial_Interfaces[] =
{
	(PTR) Serial_Manager_Tags,
	(PTR) NULL
};

SEC_RODATA static const struct TagItem Serial_CreateTags[] =
{
	{ CLT_DataSize, (Tag) sizeof( struct SerialBase ) },
	{ CLT_InitFunc, (Tag) Serial_Manager_Init },
	{ CLT_Interfaces, (Tag) Serial_Interfaces },
	{ TAG_END, (Tag) 0 }
};

// --

SEC_CODE S32 Serial_CreateDevice( struct SerialContext *ctx )
{
struct SerialBase *base;

	if ( ctx->Device )
	{
		return ( TRUE );
	}

	ObtainSemaphore( & ctx->Lock );

	if ( ctx->Device )
	{
		ReleaseSemaphore( & ctx->Lock );

		return ( TRUE );
	}

	base = (PTR) CreateLibrary( (PTR) Serial_CreateTags );

	if ( ! base )
	{
		ReleaseSemaphore( & ctx->Lock );

		return ( FALSE );
	}

	base->Context = ctx;
	InitSemaphore( & base->Semaphore );
	ctx->Device = base;

	AddDevice( (PTR) base );
	ReleaseSemaphore( & ctx->Lock );

	return ( TRUE );
}

// --
