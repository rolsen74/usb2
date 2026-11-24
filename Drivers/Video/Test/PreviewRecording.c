
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdio.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/usb2.h>
#include <proto/intuition.h>
#include "PreviewRecording.h"

// --

S32
PreviewRecording_Stage( struct PreviewRecording *recording, const char *destination, const char *filename )
{
	char parent[PREVIEW_PATH_SIZE];
	char full[PREVIEW_PATH_SIZE];
	char name[32];
	const char *leaf;
	BPTR lock;
	U32 index;
	S32 length;
	S32 resolved;

	leaf = FilePart( destination );
	length = leaf - destination;

	if ( ( ! *leaf ) || ( length < 0 ) || ( length >= (S32) sizeof( parent ) ) )
	{
		return ( FALSE );
	}

	memcpy( parent, destination, length );
	parent[length] = 0;
	recording->Drawer[0] = 0;

	lock = Lock( parent, ACCESS_READ );

	if ( ! lock )
	{
		return ( FALSE );
	}

	resolved = NameFromLock( lock, full, sizeof( full ) );

	UnLock( lock );

	if ( ! resolved )
	{
		return ( FALSE );
	}

	strlcpy( recording->Destination, full, sizeof( recording->Destination ) );

	if ( ! AddPart( recording->Destination, leaf, sizeof( recording->Destination ) ) )
	{
		return ( FALSE );
	}

	for ( index = 0; index < 10000; index++ )
	{
		snprintf( name, sizeof( name ), "USB2Capture-%04lu", (unsigned long) index );

		strlcpy( recording->Drawer, full, sizeof( recording->Drawer ) );

		if ( ! AddPart( recording->Drawer, name, sizeof( recording->Drawer ) ) )
		{
			break;
		}

		strlcpy( recording->Path, recording->Drawer, sizeof( recording->Path ) );

		if ( ! AddPart( recording->Path, filename, sizeof( recording->Path ) ) )
		{
			break;
		}

		lock = CreateDir( recording->Drawer );

		if ( lock )
		{
			recording->DrawerLock = lock;

			return ( TRUE );
		}

		if ( IoErr() != ERROR_OBJECT_EXISTS )
		{
			break;
		}
	}

	recording->Drawer[0] = 0;

	return ( FALSE );
}

// --

void
PreviewRecording_Finish( struct PreviewRecording *recording, struct Window *window, const char *kind,
	enum PreviewRecordingResult result )
{
	struct EasyStruct request;
	struct ExamineData *entry;

	const char *message;
	const char *path;
	BPTR lock;

	path = recording->Path;

	if ( ! recording->Drawer[0] )
	{
		message = "Could not reserve a location for the %s recording.\n%s";
		path = "";
	}
	else if ( result == PREVIEW_RECORD_EMPTY )
	{
		message = "No %s data was recorded.\n%s";
		path = "";

		UnLock( recording->DrawerLock );

		recording->DrawerLock = 0;

		Delete( recording->Drawer );
	}
	else if ( result == PREVIEW_RECORD_FAILED )
	{
		message = "%s recording could not be finalized.\nStaged path: %s";
	}
	else
	{
		lock = Lock( recording->Destination, ACCESS_READ );

		if ( lock )
		{
			UnLock( lock );
		}
		else if ( ( result == PREVIEW_RECORD_SAVED ) && ( IoErr() == ERROR_OBJECT_NOT_FOUND ) &&
			( Rename( recording->Path, recording->Destination ) ) )
		{
			path = recording->Destination;

			entry = ExamineObjectTags( EX_StringName, recording->Destination, TAG_END );

			if ( entry )
			{
				// Some handlers may move the source into a newly created drawer.
				if ( EXD_IS_DIRECTORY( entry ) )
				{
					AddPart( recording->Destination, FilePart( recording->Path ), sizeof( recording->Destination ) );
				}

				FreeDosObject( DOS_EXAMINEDATA, entry );
			}

			UnLock( recording->DrawerLock );

			recording->DrawerLock = 0;

			Delete( recording->Drawer );
		}

		message = result == PREVIEW_RECORD_PARTIAL ? "Recovered a partial %s recording at:\n%s" :
			"Saved %s recording at:\n%s";
	}

	if ( recording->DrawerLock )
	{
		UnLock( recording->DrawerLock );

		recording->DrawerLock = 0;
	}

	request = (struct EasyStruct) {
		.es_StructSize = sizeof( request ),
		.es_Title = "USB2VideoTest",
		.es_TextFormat = (STR) message,
		.es_GadgetFormat = "OK"
	};

	Printf( message, kind, path );

	Printf( "\n" );

	if ( path != recording->Destination )
	{
		EasyRequest( window, & request, NULL, kind, path );
	}
}

// --
