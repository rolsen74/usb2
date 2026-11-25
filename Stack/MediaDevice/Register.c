
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "MediaDevice.h"

// --

SEC_CODE BOOL
MediaDevice_Register( struct USB2MediaIFace *Self, struct Device *device )
{
	struct USB2MediaDevice *base;
	struct DeviceManagerInterface *manager;
	struct ExecIFace *IExec;

	BOOL registered;

	base = (PTR) Self->Data.LibBase;
	IExec = base->IExec;

	if ( ( ! device ) || ( device == (PTR) base ) )
	{
		return ( FALSE );
	}

	manager = (PTR) IExec->GetInterface( (PTR) device, "__device", 1, NULL );

	if ( ! manager )
	{
		return ( FALSE );
	}

	IExec->ObtainSemaphore( & base->Lock );

	registered = ! base->Backend;

	if ( registered )
	{
		base->Backend = manager;
	}

	IExec->ReleaseSemaphore( & base->Lock );

	if ( ! registered )
	{
		IExec->DropInterface( (PTR) manager );
	}

	return ( registered );
}

// --

SEC_CODE struct DeviceManagerInterface *
MediaDevice_Backend( struct USB2MediaDevice *base )
{
	struct DeviceManagerInterface *backend;
	struct ExecIFace *IExec;

	IExec = base->IExec;

	IExec->ObtainSemaphore( & base->Lock );

	backend = base->Backend;

	IExec->ReleaseSemaphore( & base->Lock );

	return ( backend );
}

// --
