
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "Monitor.h"

#include <gadgets/button.h>
#include <gadgets/texteditor.h>
#include <stdio.h>
#include <string.h>

// --

static void
StoreRead( struct Monitor *monitor )
{
	struct IOStdReq *io;

	U32 bytes;

	io = & monitor->Serial->IOSer;
	bytes = MIN( io->io_Actual, io->io_Length );
	bytes = MIN( bytes, sizeof( monitor->Input ) );
	bytes = MIN( bytes, CAPTURE_LIMIT - monitor->Length );

	if ( bytes )
	{
		memcpy( monitor->Data + monitor->Length, monitor->Input, bytes );

		monitor->Length += bytes;
		monitor->Dirty = TRUE;
	}
}

// --

static void
SendRead( struct Monitor *monitor, U32 length )
{
	struct IOStdReq *io;

	io = & monitor->Serial->IOSer;

	io->io_Command = CMD_READ;
	io->io_Data = monitor->Input;
	io->io_Length = MIN( length, CAPTURE_LIMIT - monitor->Length );
	io->io_Actual = 0;
	io->io_Flags = 0;
	monitor->Pending = TRUE;

	SendIO( (PTR) io );
}

// --

void
Capture_Stop( struct Monitor *monitor )
{
	struct IOStdReq *io;

	if ( ! monitor->Capturing )
	{
		return;
	}

	io = & monitor->Serial->IOSer;

	if ( monitor->Pending )
	{
		if ( ! CheckIO( (PTR) io ) )
		{
			AbortIO( (PTR) io );
		}

		WaitIO( (PTR) io );

		monitor->Pending = FALSE;

		if ( io->io_Command == CMD_READ )
		{
			StoreRead( monitor );
		}
	}

	CloseDevice( (PTR) io );

	monitor->Capturing = FALSE;
	monitor->Dirty = TRUE;
}

// --

S32
Capture_Start( struct Monitor *monitor, const struct LineSettings *settings )
{
	struct IOExtSer *ser;
	struct IOStdReq *io;

	S32 error;

	monitor->Notice = NULL;
	monitor->Failure = NULL;

	if ( monitor->Length == CAPTURE_LIMIT )
	{
		monitor->Notice = "Capture buffer full. Save or clear it to continue.";
		monitor->Dirty = TRUE;

		return ( IOERR_OPENFAIL );
	}

	ser = monitor->Serial;

	memset( ser, 0, sizeof( *ser ) );

	io = & ser->IOSer;
	io->io_Message.mn_Length = sizeof( *ser );
	io->io_Message.mn_ReplyPort = monitor->SerialPort;
	ser->io_SerFlags = SERF_XDISABLED;

	error = OpenDevice( USB2_SERIAL_DEVICE, settings->Unit, (PTR) ser, 0 );

	if ( error )
	{
		monitor->Failure = "Could not open serial unit";
		monitor->Error = error;
		monitor->Dirty = TRUE;

		return ( error );
	}

	ser->io_Baud = settings->Baud;
	ser->io_ReadLen = settings->DataBits;
	ser->io_WriteLen = settings->DataBits;
	ser->io_StopBits = settings->StopBits;
	ser->io_SerFlags = SERF_XDISABLED;
	ser->io_ExtFlags = 0;

	if ( ( settings->Parity == 1 ) || ( settings->Parity == 2 ) )
	{
		ser->io_SerFlags |= SERF_PARTY_ON | ( settings->Parity == 1 ? SERF_PARTY_ODD : 0 );
	}
	else if ( settings->Parity )
	{
		ser->io_ExtFlags = SEXTF_MSPON | ( settings->Parity == 3 ? SEXTF_MARK : 0 );
	}

	io->io_Command = SDCMD_SETPARAMS;

	error = DoIO( (PTR) ser );

	if ( error )
	{
		CloseDevice( (PTR) ser );

		monitor->Failure = "Serial settings rejected";
		monitor->Error = error;
		monitor->Dirty = TRUE;

		return ( error );
	}

	monitor->Capturing = TRUE;
	monitor->Dirty = TRUE;

	SendRead( monitor, 1 );

	return ( 0 );
}

// --

void
Capture_Handle( struct Monitor *monitor )
{
	struct IOStdReq *io;

	U32 available;

	if ( ( ! monitor->Pending ) || ( ! GetMsg( monitor->SerialPort ) ) )
	{
		return;
	}

	monitor->Pending = FALSE;
	io = & monitor->Serial->IOSer;

	if ( io->io_Command == CMD_READ )
	{
		StoreRead( monitor );
	}

	if ( ( io->io_Error ) && ( io->io_Error != SerErr_DetectedBreak ) )
	{
		monitor->Failure = "Serial capture stopped";
		monitor->Error = io->io_Error;

		Capture_Stop( monitor );

		return;
	}

	if ( monitor->Length == CAPTURE_LIMIT )
	{
		monitor->Notice = "Capture buffer full. Save or clear it to continue.";

		Capture_Stop( monitor );

		return;
	}

	if ( io->io_Command == SDCMD_QUERY )
	{
		if ( monitor->Serial->io_Status & IO_STATF_OVERRUN )
		{
			monitor->Overruns++;
			monitor->Dirty = TRUE;
		}

		available = MIN( io->io_Actual, (U32) sizeof( monitor->Input ) );

		SendRead( monitor, available ? available : 1 );
	}
	else
	{
		io->io_Command = SDCMD_QUERY;
		io->io_Length = 0;
		io->io_Data = NULL;
		io->io_Actual = 0;
		io->io_Flags = 0;
		monitor->Pending = TRUE;

		SendIO( (PTR) io );
	}
}

// --

void
Capture_Refresh( struct Monitor *monitor )
{
	ULONG entries;
	ULONG visible;

	U32 start;
	U32 index;
	U32 used;
	U8 value;

	if ( ! monitor->Dirty )
	{
		return;
	}

	start = monitor->Length - MIN( monitor->Length, PREVIEW_LIMIT );
	used = 0;

	for( index = start; index < monitor->Length; index++ )
	{
		value = monitor->Data[index];

		if ( value == '\r' )
		{
			monitor->Preview[used++] = '\n';
		}
		else if ( ( value == '\n' ) && ( index ) && ( monitor->Data[index - 1] == '\r' ) && ( index != start ) )
		{
			continue;
		}
		else
		{
			monitor->Preview[used++] = ( ( value == '\n' ) || ( value == '\t' ) ||
				( ( value >= 0x20 ) && ( value <= 0x7e ) ) ) ? value : '.';
		}
	}

	monitor->Preview[used] = 0;

	RefreshSetGadgetAttrs( (PTR) monitor->Editor, monitor->Window, NULL, GA_TEXTEDITOR_Contents, monitor->Preview, TAG_END );

	entries = 0;
	visible = 0;

	GetAttr( GA_TEXTEDITOR_Prop_Entries, monitor->Editor, & entries );
	GetAttr( GA_TEXTEDITOR_Prop_Visible, monitor->Editor, & visible );

	if ( entries > visible )
	{
		RefreshSetGadgetAttrs( (PTR) monitor->Editor, monitor->Window, NULL,
			GA_TEXTEDITOR_Prop_First, entries - visible, TAG_END );
	}

	if ( monitor->Failure )
	{
		snprintf( monitor->StatusText, sizeof( monitor->StatusText ), "%s (%ld). %lu bytes captured.",
			monitor->Failure, (long) monitor->Error, (unsigned long) monitor->Length );
	}
	else if ( monitor->Notice )
	{
		snprintf( monitor->StatusText, sizeof( monitor->StatusText ), "%s  |  %lu bytes",
			monitor->Notice, (unsigned long) monitor->Length );
	}
	else
	{
		snprintf( monitor->StatusText, sizeof( monitor->StatusText ), "%s  |  %lu bytes  |  Overrun reports %lu",
			monitor->Capturing ? "Capturing" : "Stopped", (unsigned long) monitor->Length, (unsigned long) monitor->Overruns );
	}

	RefreshSetGadgetAttrs( (PTR) monitor->Status, monitor->Window, NULL, GA_Text, monitor->StatusText, TAG_END );
	RefreshSetGadgetAttrs( (PTR) monitor->CaptureButton, monitor->Window, NULL,
		GA_Text, monitor->Capturing ? "Stop" : "Start", TAG_END );

	monitor->Dirty = FALSE;
}

// --

BOOL
Monitor_Save( struct Monitor *monitor, const char *path )
{
	BPTR file;

	LONG written;
	BOOL closed;

	file = Open( path, MODE_NEWFILE );

	if ( ! file )
	{
		return ( FALSE );
	}

	written = Write( file, monitor->Data, monitor->Length );

	closed = Close( file );

	return ( ( written == (LONG) monitor->Length ) && ( closed ) );
}

// --
