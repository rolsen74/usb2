
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

S32
_fs_FileCache_Create( struct FS_ObjLock *lock, S32 mode )
{
	struct DevProc *target;

	BPTR directory;
	STR folder;
	STR name;
	S32 error;

	// A PTP: assignment would recurse while the filesystem semaphore is held.
	target = GetDeviceProc( "T:", NULL );

	if ( ! target )
	{
		error = IoErr();

		return ( error ? error : ERROR_DEVICE_NOT_MOUNTED );
	}

	if ( target->dvp_Port == lock->Lock.fl_Port )
	{
		FreeDeviceProc( target );

		return ( ERROR_OBJECT_WRONG_TYPE );
	}

	FreeDeviceProc( target );

	folder = ASPrintf( "T:USB2-PTP-%08lx", GetUniqueID() );

	if ( ! folder )
	{
		return ( ERROR_NO_FREE_STORE );
	}

	// Only use a newly created directory, so a cache cannot replace an existing file.
	directory = CreateDir( folder );

	if ( ! directory )
	{
		error = IoErr();

		FreeVec( folder );

		return ( error ? error : ERROR_DISK_FULL );
	}

	UnLock( directory );

	name = ASPrintf( "%s/object", folder );

	if ( name )
	{
		lock->Cache = Open( name, mode );
	}

	if ( ! lock->Cache )
	{
		error = name ? IoErr() : ERROR_NO_FREE_STORE;

		Delete( folder );

		FreeVec( folder );

		FreeVec( name );

		return ( error ? error : ERROR_DISK_FULL );
	}

	lock->CacheName = name;

	FreeVec( folder );

	return ( 0 );
}

// --
