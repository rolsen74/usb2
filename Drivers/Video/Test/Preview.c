
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdio.h>
#include <stdlib.h>
#include <exec/errors.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <datatypes/pictureclass.h>
#include <interfaces/timer.h>
#include <devices/usb2.h>
#include "../inc/VideoDevice.h"
#include "Preview.h"

// --

struct IntuitionIFace *IIntuition;
struct GadToolsIFace *IGadTools;
struct GraphicsIFace *IGraphics;
struct DataTypesIFace *IDataTypes;

#define PREVIEW_START 1
#define PREVIEW_SIZE 2
#define PREVIEW_SPEED 3
#define PREVIEW_FORMAT 4
#define PREVIEW_CONTROL_WIDTH 120
#define PREVIEW_CONTROL_GAP 4
#define PREVIEW_WIDTH 640
#define PREVIEW_HEIGHT 480
#define PREVIEW_INSET 4
#define PREVIEW_BEVEL 3

struct PreviewMode
{
	U32 Number;
	U32 Format;
	U32 SizeIndex;
	U32 Interval;
	U32 Width;
	U32 Height;
	char Label[24];
};

struct PreviewSize
{
	U32 Format;
	U32 Choice;
	U32 Width;
	U32 Height;
	char Label[24];
};

struct Preview
{
	struct Window *Window;
	struct BitMap *Bitmap;
	struct Gadget *FormatGadget;
	struct Gadget *SizeGadget;
	struct Gadget *SpeedGadget;
	Object *Picture;
	U8 *PictureSource;
	U8 *RGB;
	struct PreviewMode *Modes;
	struct PreviewSize *Sizes;
	STR FormatLabels[3];
	U32 Formats[2];
	U32 FormatCount;
	STR *SizeLabels[2];
	U32 *SizeChoices[2];
	U32 SizeCounts[2];
	STR *SpeedLabels[2];
	U32 *SpeedModes;
	U32 Width;
	U32 Height;
	U32 MaxWidth;
	U32 MaxHeight;
	U32 Left;
	U32 Top;
	U32 ModeCount;
	U32 SizeCount;
	U32 SpeedCount;
	U32 SpeedActive;
	U32 SpeedSlot;
	U32 ActiveMode;
	char CaptureTitle[2][96];
	U32 TitleSlot;
};

// --

static PTR
OpenPreviewLibrary( const char *name )
{
	struct Library *base;
	struct Interface *iface;

	base = OpenLibrary( name, 50 );

	if ( ! base )
	{
		return ( NULL );
	}

	iface = GetInterface( base, "main", 1, NULL );

	if ( ! iface )
	{
		CloseLibrary( base );
	}

	return ( iface );
}

// --

static void
ClosePreviewLibrary( struct Interface *iface )
{
	struct Library *base;

	if ( iface )
	{
		base = iface->Data.LibBase;

		DropInterface( iface );

		CloseLibrary( base );
	}
}

// --

static void
ClearPreview( const struct Preview *preview )
{
	EraseRect( preview->Window->RPort, preview->Left, preview->Top,
		preview->Left + preview->MaxWidth - 1, preview->Top + preview->MaxHeight - 1 );
}

// --

static void
DrawPreview( const struct Preview *preview, S32 raw )
{
	U32 left;
	U32 top;

	left = preview->Left + ( preview->MaxWidth - preview->Width ) / 2;
	top = preview->Top + ( preview->MaxHeight - preview->Height ) / 2;

	ClearPreview( preview );

	if ( raw )
	{
		WritePixelArray( preview->RGB, 0, 0, preview->Width * 3, PIXF_R8G8B8,
			preview->Window->RPort, left, top, preview->Width, preview->Height );
	}
	else
	{
		BltBitMapRastPort( preview->Bitmap, 0, 0, preview->Window->RPort,
			left, top, preview->Width, preview->Height, 0xc0 );
	}
}

// --

static void
ReleasePreviewPicture( struct Preview *preview )
{
	if ( preview->Picture )
	{
		WaitBlit();

		DisposeDTObject( preview->Picture );

		preview->Picture = NULL;
		preview->Bitmap = NULL;

		FreeVec( preview->PictureSource );

		preview->PictureSource = NULL;
	}
}

// --

static S32
DecodeFrame( struct Preview *preview, U8 *frame, U32 size )
{
	struct BitMapHeader *header;
	struct BitMap *bitmap;
	struct gpLayout layout;
	Object *picture;

	U8 *source;

	// Keep the datatype's source stable while capture reuses its frame buffers.
	source = AllocVecTags( size, TAG_END );

	if ( ! source )
	{
		return ( FALSE );
	}

	CopyMem( frame, source, size );

	picture = NewDTObject( NULL, DTA_SourceType, DTST_MEMORY,
		DTA_SourceAddress, source, DTA_SourceSize, size, DTA_GroupID, GID_PICTURE,
		PDTA_DestMode, PMODE_V43, PDTA_Screen, preview->Window->WScreen, PDTA_Remap, TRUE,
		PDTA_FreeSourceBitMap, TRUE, TAG_END );

	if ( ! picture )
	{
		FreeVec( source );

		return ( FALSE );
	}

	header = NULL;
	bitmap = NULL;

	GetDTAttrs( picture, PDTA_BitMapHeader, & header, TAG_END );

	if ( ( ! header ) || ( ! header->bmh_Width ) || ( ! header->bmh_Height ) )
	{
		goto failed;
	}

	if ( ( header->bmh_Width != preview->Width ) || ( header->bmh_Height != preview->Height ) )
	{
		struct pdtScale scale;

		scale.MethodID = PDTM_SCALE;
		scale.ps_NewWidth = preview->Width;
		scale.ps_NewHeight = preview->Height;
		scale.ps_Flags = 0;

		if ( ! IDoMethodA( picture, (Msg) & scale ) )
		{
			goto failed;
		}
	}

	layout.MethodID = DTM_PROCLAYOUT;
	layout.gpl_GInfo = NULL;
	layout.gpl_Initial = TRUE;

	if ( ! IDoMethodA( picture, (Msg) & layout ) )
	{
		goto failed;
	}

	GetDTAttrs( picture, PDTA_DestBitMap, & bitmap, TAG_END );

	if ( ! bitmap )
	{
		goto failed;
	}

	ReleasePreviewPicture( preview );

	preview->Picture = picture;
	preview->Bitmap = bitmap;
	preview->PictureSource = source;

	return ( TRUE );

failed:
	DisposeDTObject( picture );

	FreeVec( source );

	return ( FALSE );
}

// --

static void
FinishRequest( struct IORequest *request )
{
	if ( ! CheckIO( request ) )
	{
		AbortIO( request );
	}

	WaitIO( request );
}

// --

static S32
RenderPreviewFrame( struct Preview *preview, const struct VideoFormat *format, U8 *frame, U32 size )
{
	S32 raw;

	raw = format->Format == VIDEO_FORMAT_YUY2;

	if ( ( ! size ) || ( size > format->MaxFrameSize ) )
	{
		return ( FALSE );
	}

	if ( raw )
	{
		if ( ! preview->RGB )
		{
			preview->RGB = AllocVecTags( preview->MaxWidth * preview->MaxHeight * 3, TAG_END );
		}

		if ( ( ! preview->RGB ) ||
			( ! VideoTest_YUY2ToRGB( frame, size, format->Width, format->Height,
				preview->RGB, preview->Width, preview->Height ) ) )
		{
			return ( FALSE );
		}
	}
	else if ( ! DecodeFrame( preview, frame, size ) )
	{
		return ( FALSE );
	}

	DrawPreview( preview, raw );

	return ( TRUE );
}

// --

static void
StartFrame( struct IOStdReq *ioreq, struct TimeRequest *timer, U8 *frame, U32 size )
{
	ioreq->io_Command = CMD_READ;
	ioreq->io_Data = frame;
	ioreq->io_Length = size;
	timer->Request.io_Command = TR_ADDREQUEST;
	timer->Request.io_Flags = 0;
	timer->Time.Seconds = 10;
	timer->Time.Microseconds = 0;

	SendIO( & timer->Request );

	SendIO( (PTR) ioreq );
}

// --

static U64
PreviewTime( struct TimerIFace *clock )
{
	struct TimeVal time;

	clock->GetUpTime( & time );

	return ( (U64) time.Seconds * 1000000 + time.Microseconds );
}

// --

static void
ShowPreviewRate( struct Preview *preview, const struct VideoFormat *format, U32 rate )
{
	U32 fps;
	STR title;

	fps = ( 100000000UL + format->FrameInterval / 2 ) / (U64) format->FrameInterval;
	title = preview->CaptureTitle[preview->TitleSlot];

	if ( rate )
	{
		snprintf( title, sizeof( preview->CaptureTitle[0] ),
			"USB2VideoTest - %lux%lu - preview %lu.%lu / camera %lu.%lu fps",
			(unsigned long) format->Width, (unsigned long) format->Height,
			(unsigned long) ( rate / 10 ), (unsigned long) ( rate % 10 ),
			(unsigned long) ( fps / 10 ), (unsigned long) ( fps % 10 ) );
	}
	else
	{
		snprintf( title, sizeof( preview->CaptureTitle[0] ),
			"USB2VideoTest - %lux%lu - camera %lu.%lu fps",
			(unsigned long) format->Width, (unsigned long) format->Height,
			(unsigned long) ( fps / 10 ), (unsigned long) ( fps % 10 ) );
	}

	SetWindowTitles( preview->Window, title, (CONST_STRPTR) -1 );

	preview->TitleSlot ^= 1;
}

// --

static void
SizePreview( struct Preview *preview, const struct VideoFormat *format )
{
	preview->Width = MIN( preview->MaxWidth, format->Width );
	preview->Height = MAX( 1, (U64) preview->Width * format->Height / format->Width );

	if ( preview->Height > preview->MaxHeight )
	{
		preview->Height = preview->MaxHeight;
		preview->Width = MAX( 1, (U64) preview->Height * format->Width / format->Height );
	}
}

// --

static S32
LoadPreviewModes( struct Preview *preview, struct IOStdReq *ioreq, const struct VideoFormat *active )
{
	struct VideoFormat format;
	struct PreviewMode *modes;

	U32 index;
	U32 number;
	U32 capacity;
	U32 fps;
	U32 size;
	U32 group;
	U32 distance;
	U32 best;
	S32 error;

	capacity = 0;

	for ( number = 1; ; number++ )
	{
		ioreq->io_Command = VIDEO_CMD_GET_MODE;
		ioreq->io_Data = & format;
		ioreq->io_Length = sizeof( format );
		ioreq->io_Offset = number;

		error = DoIO( (PTR) ioreq );

		if ( ( error == IOERR_BADADDRESS ) || ( error == IOERR_NOCMD ) )
		{
			break;
		}

		if ( error )
		{
			return ( FALSE );
		}

		if ( ( ( format.Format != VIDEO_FORMAT_MJPEG ) && ( format.Format != VIDEO_FORMAT_YUY2 ) ) ||
			( ! format.Width ) || ( ! format.Height ) || ( ! format.FrameInterval ) ||
			( ( format.Format == VIDEO_FORMAT_YUY2 ) && ( format.Width & 1 ) ) )
		{
			continue;
		}

		for ( index = 0; index < preview->ModeCount; index++ )
		{
			if ( ( preview->Modes[index].Format == format.Format ) &&
				( preview->Modes[index].Width == format.Width ) &&
				( preview->Modes[index].Height == format.Height ) &&
				( preview->Modes[index].Interval == format.FrameInterval ) )
			{
				break;
			}
		}

		if ( index < preview->ModeCount )
		{
			continue;
		}

		if ( preview->ModeCount == capacity )
		{
			capacity = capacity ? capacity * 2 : 16;

			modes = realloc( preview->Modes, capacity * sizeof( *modes ) );

			if ( ! modes )
			{
				return ( FALSE );
			}

			preview->Modes = modes;
		}

		index = preview->ModeCount++;
		preview->Modes[index].Number = number;
		preview->Modes[index].Format = format.Format;
		preview->Modes[index].Width = format.Width;
		preview->Modes[index].Height = format.Height;
		preview->Modes[index].Interval = format.FrameInterval;
		fps = ( 100000000UL + format.FrameInterval / 2 ) / (U64) format.FrameInterval;

		snprintf( preview->Modes[index].Label, sizeof( preview->Modes[index].Label ),
			"%lu.%lu fps",
			(unsigned long) ( fps / 10 ), (unsigned long) ( fps % 10 ) );
	}

	if ( ! preview->ModeCount )
	{
		return ( TRUE );
	}

	preview->Sizes = calloc( preview->ModeCount, sizeof( *preview->Sizes ) );
	preview->SizeLabels[0] = calloc( preview->ModeCount + 1, sizeof( STR ) );
	preview->SizeLabels[1] = calloc( preview->ModeCount + 1, sizeof( STR ) );
	preview->SizeChoices[0] = calloc( preview->ModeCount, sizeof( U32 ) );
	preview->SizeChoices[1] = calloc( preview->ModeCount, sizeof( U32 ) );
	preview->SpeedLabels[0] = calloc( preview->ModeCount + 1, sizeof( STR ) );
	preview->SpeedLabels[1] = calloc( preview->ModeCount + 1, sizeof( STR ) );
	preview->SpeedModes = calloc( preview->ModeCount, sizeof( *preview->SpeedModes ) );

	if ( ( ! preview->Sizes ) || ( ! preview->SizeLabels[0] ) || ( ! preview->SizeLabels[1] ) ||
		( ! preview->SizeChoices[0] ) || ( ! preview->SizeChoices[1] ) || ( ! preview->SpeedLabels[0] ) ||
		( ! preview->SpeedLabels[1] ) || ( ! preview->SpeedModes ) )
	{
		return ( FALSE );
	}

	best = ~0U;

	for ( index = 0; index < preview->ModeCount; index++ )
	{
		modes = & preview->Modes[index];
		group = modes->Format == VIDEO_FORMAT_YUY2;

		for ( size = 0; size < preview->SizeCount; size++ )
		{
			if ( ( preview->Sizes[size].Format == modes->Format ) &&
				( preview->Sizes[size].Width == modes->Width ) &&
				( preview->Sizes[size].Height == modes->Height ) )
			{
				break;
			}
		}

		if ( size == preview->SizeCount )
		{
			if ( ! preview->SizeCounts[group] )
			{
				preview->Formats[preview->FormatCount] = modes->Format;
				preview->FormatLabels[preview->FormatCount++] = group ? "YUY2" : "MJPEG";
				preview->FormatLabels[preview->FormatCount] = NULL;
			}

			preview->Sizes[size].Format = modes->Format;
			preview->Sizes[size].Choice = preview->SizeCounts[group];
			preview->Sizes[size].Width = modes->Width;
			preview->Sizes[size].Height = modes->Height;
			preview->SizeLabels[group][preview->SizeCounts[group]] = preview->Sizes[size].Label;
			preview->SizeChoices[group][preview->SizeCounts[group]++] = size;
			preview->SizeCount++;

			snprintf( preview->Sizes[size].Label, sizeof( preview->Sizes[size].Label ),
				"%lux%lu", (unsigned long) modes->Width, (unsigned long) modes->Height );
		}

		modes->SizeIndex = size;

		if ( ( active->Format == modes->Format ) &&
			( active->Width == modes->Width ) && ( active->Height == modes->Height ) )
		{
			distance = MAX( active->FrameInterval, modes->Interval ) - MIN( active->FrameInterval, modes->Interval );

			if ( distance < best )
			{
				best = distance;
				preview->ActiveMode = index;
			}
		}
	}

	return ( TRUE );
}

// --

static void
UpdateSpeedChoices( struct Preview *preview )
{
	U32 index;
	U32 size;

	size = preview->Modes[preview->ActiveMode].SizeIndex;
	preview->SpeedCount = 0;
	preview->SpeedActive = 0;
	preview->SpeedSlot ^= 1;

	for ( index = 0; index < preview->ModeCount; index++ )
	{
		if ( preview->Modes[index].SizeIndex != size )
		{
			continue;
		}

		if ( index == preview->ActiveMode )
		{
			preview->SpeedActive = preview->SpeedCount;
		}

		preview->SpeedLabels[preview->SpeedSlot][preview->SpeedCount] = preview->Modes[index].Label;
		preview->SpeedModes[preview->SpeedCount++] = index;
	}

	preview->SpeedLabels[preview->SpeedSlot][preview->SpeedCount] = NULL;
}

// --

static U32
ClosestSizeMode( const struct Preview *preview, U32 size, U32 interval )
{
	U32 index;
	U32 selected;
	U32 best;
	U32 distance;
	U32 candidate;

	selected = preview->ActiveMode;
	best = ~0U;

	for ( index = 0; index < preview->ModeCount; index++ )
	{
		if ( preview->Modes[index].SizeIndex == size )
		{
			candidate = preview->Modes[index].Interval;
			distance = MAX( interval, candidate ) - MIN( interval, candidate );

			if ( distance < best )
			{
				best = distance;
				selected = index;
			}
		}
	}

	return ( selected );
}

// --

static U32
ClosestFormatMode( const struct Preview *preview, const struct VideoFormat *current, U32 format )
{
	const struct PreviewSize *size;

	U32 index;
	U32 selected;
	U32 distance;
	U32 best;
	U32 group;

	group = format == VIDEO_FORMAT_YUY2;
	selected = preview->SizeChoices[group][0];
	best = ~0U;

	for ( index = 0; index < preview->SizeCounts[group]; index++ )
	{
		size = & preview->Sizes[preview->SizeChoices[group][index]];
		distance = MAX( size->Width, current->Width ) - MIN( size->Width, current->Width ) +
			MAX( size->Height, current->Height ) - MIN( size->Height, current->Height );

		if ( distance < best )
		{
			best = distance;
			selected = preview->SizeChoices[group][index];
		}
	}

	return ( ClosestSizeMode( preview, selected, current->FrameInterval ) );
}

// --

static void
RefreshModeChoices( struct Preview *preview )
{
	const struct PreviewSize *size;

	U32 group;
	U32 formatChoice;

	size = & preview->Sizes[preview->Modes[preview->ActiveMode].SizeIndex];
	group = size->Format == VIDEO_FORMAT_YUY2;
	formatChoice = preview->Formats[0] != size->Format;

	UpdateSpeedChoices( preview );

	GT_SetGadgetAttrs( preview->FormatGadget, preview->Window, NULL,
		GTCY_Active, formatChoice, TAG_END );

	GT_SetGadgetAttrs( preview->SizeGadget, preview->Window, NULL,
		GTCY_Labels, preview->SizeLabels[group], GTCY_Active, size->Choice, TAG_END );

	GT_SetGadgetAttrs( preview->SpeedGadget, preview->Window, NULL,
		GTCY_Labels, preview->SpeedLabels[preview->SpeedSlot], GTCY_Active, preview->SpeedActive, TAG_END );
}

// --

static S32
ChangePreviewMode( struct Preview *preview, struct IOStdReq *ioreq, struct VideoFormat *format, U32 index )
{
	S32 error;
	S32 query;

	ioreq->io_Command = VIDEO_CMD_SET_MODE;
	ioreq->io_Offset = preview->Modes[index].Number;
	ioreq->io_Data = NULL;
	ioreq->io_Length = 0;

	error = DoIO( (PTR) ioreq );

	ioreq->io_Command = VIDEO_CMD_QUERY;
	ioreq->io_Data = format;
	ioreq->io_Length = sizeof( *format );

	query = DoIO( (PTR) ioreq );

	if ( ( query ) || ( ( format->Format != VIDEO_FORMAT_MJPEG ) && ( format->Format != VIDEO_FORMAT_YUY2 ) ) ||
		( ! format->Width ) || ( ! format->Height ) || ( ! format->FrameInterval ) )
	{
		format->MaxFrameSize = 0;
	}

	if ( ( error ) || ( ! format->MaxFrameSize ) )
	{
		SetWindowTitles( preview->Window, "USB2VideoTest - Could not set camera mode", (CONST_STRPTR) -1 );

		return ( FALSE );
	}

	preview->ActiveMode = index;

	ReleasePreviewPicture( preview );

	ClearPreview( preview );

	SizePreview( preview, format );

	return ( TRUE );
}

// --

static S32
PrepareFrameBuffers( U8 **frame, U8 **spare, U32 *capacity, U32 size )
{
	if ( ! size )
	{
		return ( FALSE );
	}

	if ( size > *capacity )
	{
		FreeVec( *frame );

		FreeVec( *spare );

		*frame = AllocVecTags( size, TAG_END );

		*spare = AllocVecTags( size, TAG_END );

		*capacity = ( ( *frame ) && ( *spare ) ) ? size : 0;
	}

	return ( *capacity != 0 );
}

// --

static void
RunPreview( struct Preview *preview, struct IOStdReq *ioreq, struct TimeRequest *timer,
	struct VideoFormat *format )
{
	struct IntuiMessage *msg;
	struct Window *window;
	struct TimerIFace *clock;

	U8 *frame;
	U8 *spare;
	U8 *ready;
	U64 start;
	U64 now;
	U32 shown;
	U32 size;
	U32 mask;
	U32 signals;
	U32 event;
	U32 code;
	U32 gadget;
	U32 capacity;
	U32 mode;
	U32 group;
	U32 failures;
	S32 running;
	S32 pending;
	S32 streaming;
	S32 quit;
	S32 timedout;
	S32 modeDisabled;

	window = preview->Window;

	clock = (PTR) GetInterface( (PTR) timer->Request.io_Device, "main", 1, NULL );

	mask = ( 1UL << window->UserPort->mp_SigBit ) |
		( 1UL << ioreq->io_Message.mn_ReplyPort->mp_SigBit ) |
		( 1UL << timer->Request.io_Message.mn_ReplyPort->mp_SigBit ) | SIGBREAKF_CTRL_C;
	failures = 0;
	running = FALSE;
	pending = FALSE;
	streaming = FALSE;
	quit = FALSE;
	shown = 0;
	start = 0;
	frame = NULL;
	spare = NULL;
	capacity = 0;
	modeDisabled = FALSE;

	while( ! quit )
	{
		signals = Wait( mask );
		quit = ( signals & SIGBREAKF_CTRL_C ) != 0;

		while( ( msg = GT_GetIMsg( window->UserPort ) ) )
		{
			event = msg->Class;
			code = msg->Code;
			gadget = 0;

			if ( event == IDCMP_GADGETUP )
			{
				gadget = ( (struct Gadget *) msg->IAddress )->GadgetID;
			}

			GT_ReplyIMsg( msg );

			if ( event == IDCMP_CLOSEWINDOW )
			{
				quit = TRUE;
			}
			else if ( ( ! quit ) && ( ! running ) && ( ! pending ) &&
				( ( gadget == PREVIEW_FORMAT ) || ( gadget == PREVIEW_SIZE ) || ( gadget == PREVIEW_SPEED ) ) )
			{
				group = preview->Modes[preview->ActiveMode].Format == VIDEO_FORMAT_YUY2;

				if ( ( gadget == PREVIEW_FORMAT ) && ( code < preview->FormatCount ) )
				{
					mode = ClosestFormatMode( preview, format, preview->Formats[code] );
				}
				else if ( ( gadget == PREVIEW_SIZE ) && ( code < preview->SizeCounts[group] ) )
				{
					mode = ClosestSizeMode( preview, preview->SizeChoices[group][code], format->FrameInterval );
				}
				else if ( ( gadget == PREVIEW_SPEED ) && ( code < preview->SpeedCount ) )
				{
					mode = preview->SpeedModes[code];
				}
				else
				{
					continue;
				}

				if ( ChangePreviewMode( preview, ioreq, format, mode ) )
				{
					ShowPreviewRate( preview, format, 0 );
				}

				RefreshModeChoices( preview );
			}
			else if ( ( ! quit ) && ( gadget == PREVIEW_START ) )
			{
				running = ! running;
				failures = 0;

				if ( running )
				{
					if ( ! PrepareFrameBuffers( & frame, & spare, & capacity, format->MaxFrameSize ) )
					{
						running = FALSE;

						SetWindowTitles( window, "USB2VideoTest - Cannot start preview", (CONST_STRPTR) -1 );
						continue;
					}

					shown = 0;
					start = clock ? PreviewTime( clock ) : 0;

					ShowPreviewRate( preview, format, 0 );
				}
				else
				{
					SetWindowTitles( window, "USB2VideoTest - Stopped", (CONST_STRPTR) -1 );
				}
			}
		}

		if ( ( running ) && ( ! quit ) && ( pending ) &&
			( ( CheckIO( (PTR) ioreq ) ) || ( CheckIO( & timer->Request ) ) ) )
		{
			timedout = ! CheckIO( (PTR) ioreq );

			FinishRequest( (PTR) ioreq );

			FinishRequest( & timer->Request );

			pending = FALSE;

			if ( ( timedout ) || ( ioreq->io_Error ) )
			{
				running = FALSE;

				SetWindowTitles( window, timedout ? "USB2VideoTest - Capture timed out" :
					"USB2VideoTest - Capture failed", (CONST_STRPTR) -1 );

				if ( ! timedout )
				{
					Printf( "Preview read failed (error %ld).\n", (S32) ioreq->io_Error );
				}
			}
			else
			{
				size = ioreq->io_Actual;
				ready = frame;
				frame = spare;
				spare = ready;

				// Keep the next read active while displaying the finished frame.
				StartFrame( ioreq, timer, frame, format->MaxFrameSize );

				pending = TRUE;

				if ( RenderPreviewFrame( preview, format, ready, size ) )
				{
					failures = 0;

					if ( clock )
					{
						shown++;
						now = PreviewTime( clock );

						if ( now - start >= 1000000 )
						{
							ShowPreviewRate( preview, format, 10000000ULL * shown / ( now - start ) );

							shown = 0;
							start = now;
						}
					}
				}
				else if ( ++failures == 3 )
				{
					running = FALSE;

					SetWindowTitles( window, "USB2VideoTest - Frame decode failed", (CONST_STRPTR) -1 );

					if ( format->Format == VIDEO_FORMAT_MJPEG )
					{
						Printf( "Could not display the JPEG frames. Check DEVS:DataTypes for the JPEG datatype.\n" );
					}
					else
					{
						Printf( "Could not display the YUY2 frames.\n" );
					}
				}
			}
		}

		if ( ( pending ) && ( ( quit ) || ( ! running ) ) )
		{
			FinishRequest( (PTR) ioreq );

			FinishRequest( & timer->Request );

			pending = FALSE;
		}
		else if ( ( running ) && ( ! pending ) && ( ! quit ) )
		{
			StartFrame( ioreq, timer, frame, format->MaxFrameSize );

			pending = TRUE;
			streaming = TRUE;
		}

		if ( ( streaming ) && ( ( ! running ) || ( quit ) ) )
		{
			ioreq->io_Command = VIDEO_CMD_STOP;
			ioreq->io_Data = NULL;
			ioreq->io_Length = 0;

			DoIO( (PTR) ioreq );

			streaming = FALSE;
		}

		if ( ( preview->SizeGadget ) && ( modeDisabled != running ) )
		{
			modeDisabled = running;

			GT_SetGadgetAttrs( preview->FormatGadget, window, NULL,
				GA_Disabled, ( running || ( preview->FormatCount < 2 ) ), TAG_END );

			GT_SetGadgetAttrs( preview->SizeGadget, window, NULL, GA_Disabled, running, TAG_END );

			GT_SetGadgetAttrs( preview->SpeedGadget, window, NULL, GA_Disabled, running, TAG_END );
		}
	}

	FreeVec( frame );

	FreeVec( spare );

	if ( clock )
	{
		DropInterface( (PTR) clock );
	}
}

// --

static void
PlaceControl( struct NewGadget *gadget, const struct Window *window, U32 columns, U32 slot )
{
	U32 width;
	U32 gutter;

	width = window->Width - window->BorderLeft - window->BorderRight;
	gutter = ( width - columns * gadget->ng_Width - ( columns - 1 ) * PREVIEW_CONTROL_GAP ) / 2;
	gadget->ng_LeftEdge = window->BorderLeft + gutter + ( slot % columns ) *
		( gadget->ng_Width + PREVIEW_CONTROL_GAP );
	gadget->ng_TopEdge = window->BorderTop + PREVIEW_CONTROL_GAP + ( slot / columns ) *
		( gadget->ng_Height + PREVIEW_CONTROL_GAP );
}

// --

S32
VideoTest_Preview( struct IOStdReq *ioreq, struct TimeRequest *timer,
	const struct VideoFormat *format )
{
	struct Screen *screen;
	struct Gadget *gadgets;
	struct Gadget *context;
	struct NewGadget button;
	struct VideoFormat selected;
	struct Preview preview;

	PTR visual;
	S32 result;
	S32 width;
	S32 height;
	S32 controls;
	U32 columns;
	U32 rows;
	U32 gadgetHeight;
	U32 group;

	result = RETURN_FAIL;
	selected = *format;
	preview.Modes = NULL;
	preview.Sizes = NULL;
	preview.SizeLabels[0] = NULL;
	preview.SizeLabels[1] = NULL;
	preview.SizeChoices[0] = NULL;
	preview.SizeChoices[1] = NULL;
	preview.SizeCounts[0] = 0;
	preview.SizeCounts[1] = 0;
	preview.FormatCount = 0;
	preview.SpeedLabels[0] = NULL;
	preview.SpeedLabels[1] = NULL;
	preview.SpeedModes = NULL;
	preview.FormatGadget = NULL;
	preview.SizeGadget = NULL;
	preview.SpeedGadget = NULL;
	preview.ModeCount = 0;
	preview.SizeCount = 0;
	preview.SpeedCount = 0;
	preview.SpeedSlot = 0;
	preview.ActiveMode = 0;

	if ( ( format->Format != VIDEO_FORMAT_MJPEG ) && ( format->Format != VIDEO_FORMAT_YUY2 ) )
	{
		Printf( "The preview requires MJPEG or YUY2.\n" );

		return ( result );
	}

	if ( ! format->FrameInterval )
	{
		Printf( "The camera did not report a valid frame interval.\n" );

		return ( result );
	}

	IIntuition = OpenPreviewLibrary( "intuition.library" );
	IGraphics = OpenPreviewLibrary( "graphics.library" );
	IGadTools = OpenPreviewLibrary( "gadtools.library" );
	IDataTypes = OpenPreviewLibrary( "datatypes.library" );

	if ( ( ! IIntuition ) || ( ! IGraphics ) || ( ! IGadTools ) || ( ! IDataTypes ) )
	{
		Printf( "Could not open the preview libraries.\n" );
		goto close_libraries;
	}

	if ( ! LoadPreviewModes( & preview, ioreq, format ) )
	{
		Printf( "Could not load camera modes for the preview.\n" );
		goto close_libraries;
	}

	screen = LockPubScreen( NULL );

	if ( ! screen )
	{
		Printf( "Could not lock the public screen.\n" );
		goto close_libraries;
	}

	width = screen->Width - screen->WBorLeft - screen->WBorRight - 4;
	gadgetHeight = screen->Font->ta_YSize + 6;
	columns = 4;
	rows = 1;

	if ( ( preview.ModeCount ) && ( width < 4 * PREVIEW_CONTROL_WIDTH + 5 * PREVIEW_CONTROL_GAP ) )
	{
		columns = 2;
		rows = 2;
	}

	controls = rows * gadgetHeight + ( rows + 1 ) * PREVIEW_CONTROL_GAP;
	height = screen->Height - screen->BarHeight - screen->WBorTop -
		screen->WBorBottom - screen->Font->ta_YSize - controls - 4 - 2 * PREVIEW_INSET;

	if ( ( width < 2 * PREVIEW_CONTROL_WIDTH + 3 * PREVIEW_CONTROL_GAP ) || ( height < 1 ) ||
		( ! format->Width ) || ( ! format->Height ) )
	{
		Printf( "Cannot fit the preview on the public screen.\n" );
		goto unlock_screen;
	}

	width = MIN( width, PREVIEW_WIDTH + 2 * PREVIEW_INSET );
	height = MIN( height, PREVIEW_HEIGHT );

	preview.Window = OpenWindowTags( NULL, WA_PubScreen, screen,
		WA_InnerWidth, width,
		WA_InnerHeight, height + controls + 2 * PREVIEW_INSET,
		WA_Title, "USB2VideoTest - Stopped", WA_DragBar, TRUE, WA_DepthGadget, TRUE,
		WA_CloseGadget, TRUE, WA_Activate, TRUE, WA_SmartRefresh, TRUE, WA_NoCareRefresh, TRUE,
		WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP, TAG_END );

	if ( ! preview.Window )
	{
		Printf( "Could not open the preview window.\n" );
		goto unlock_screen;
	}

	visual = GetVisualInfo( screen, TAG_END );

	if ( ! visual )
	{
		goto close_window;
	}

	gadgets = NULL;

	context = CreateContext( & gadgets );

	if ( ! context )
	{
		goto free_gadgets;
	}

	width = preview.Window->Width - preview.Window->BorderLeft - preview.Window->BorderRight;
	button.ng_Width = ( width - ( columns + 1 ) * PREVIEW_CONTROL_GAP ) / columns;
	button.ng_Height = gadgetHeight;
	button.ng_GadgetText = "Start / Stop";
	button.ng_TextAttr = screen->Font;
	button.ng_GadgetID = PREVIEW_START;
	button.ng_Flags = PLACETEXT_IN;
	button.ng_VisualInfo = visual;
	button.ng_UserData = NULL;

	PlaceControl( & button, preview.Window, columns, 0 );

	context = CreateGadget( BUTTON_KIND, context, & button, TAG_END );

	if ( ! context )
	{
		goto free_gadgets;
	}

	if ( preview.ModeCount )
	{
		group = preview.Modes[preview.ActiveMode].Format == VIDEO_FORMAT_YUY2;

		UpdateSpeedChoices( & preview );

		button.ng_GadgetText = NULL;
		button.ng_GadgetID = PREVIEW_FORMAT;
		button.ng_Flags = 0;

		PlaceControl( & button, preview.Window, columns, 1 );

		preview.FormatGadget = CreateGadget( CYCLE_KIND, context, & button,
			GTCY_Labels, preview.FormatLabels, GTCY_Active,
			preview.Formats[0] != preview.Modes[preview.ActiveMode].Format,
			GA_Disabled, preview.FormatCount < 2, TAG_END );

		if ( ! preview.FormatGadget )
		{
			goto free_gadgets;
		}

		button.ng_GadgetID = PREVIEW_SIZE;

		PlaceControl( & button, preview.Window, columns, 2 );

		preview.SizeGadget = CreateGadget( CYCLE_KIND, preview.FormatGadget, & button,
			GTCY_Labels, preview.SizeLabels[group],
			GTCY_Active, preview.Sizes[preview.Modes[preview.ActiveMode].SizeIndex].Choice, TAG_END );

		if ( ! preview.SizeGadget )
		{
			goto free_gadgets;
		}

		button.ng_GadgetID = PREVIEW_SPEED;

		PlaceControl( & button, preview.Window, columns, 3 );

		preview.SpeedGadget = CreateGadget( CYCLE_KIND, preview.SizeGadget, & button,
			GTCY_Labels, preview.SpeedLabels[preview.SpeedSlot], GTCY_Active, preview.SpeedActive, TAG_END );

		if ( ! preview.SpeedGadget )
		{
			goto free_gadgets;
		}
	}
	else
	{
		Printf( "Camera mode selection requires the updated Video.Driver.\n" );
	}

	AddGList( preview.Window, gadgets, (UWORD) -1, -1, NULL );

	RefreshGList( gadgets, preview.Window, NULL, -1 );

	GT_RefreshWindow( preview.Window, NULL );

	preview.Picture = NULL;
	preview.PictureSource = NULL;
	preview.RGB = NULL;
	preview.Bitmap = NULL;
	preview.Left = preview.Window->BorderLeft + PREVIEW_INSET;
	preview.Top = preview.Window->BorderTop + controls + PREVIEW_INSET;
	preview.MaxWidth = preview.Window->Width - preview.Left - preview.Window->BorderRight - PREVIEW_INSET;
	preview.MaxHeight = preview.Window->Height - preview.Top - preview.Window->BorderBottom - PREVIEW_INSET;
	preview.TitleSlot = 0;

	SizePreview( & preview, format );

	DrawBevelBox( preview.Window->RPort, preview.Left - PREVIEW_BEVEL, preview.Top - PREVIEW_BEVEL,
		preview.MaxWidth + 2 * PREVIEW_BEVEL, preview.MaxHeight + 2 * PREVIEW_BEVEL,
		GT_VisualInfo, visual, GTBB_Recessed, TRUE, TAG_END );

	ClearPreview( & preview );

	RunPreview( & preview, ioreq, timer, & selected );

	ReleasePreviewPicture( & preview );

	FreeVec( preview.RGB );

	RemoveGList( preview.Window, gadgets, -1 );

	result = RETURN_OK;

free_gadgets:
	FreeGadgets( gadgets );

	FreeVisualInfo( visual );

close_window:
	CloseWindow( preview.Window );

unlock_screen:
	UnlockPubScreen( NULL, screen );

close_libraries:
	ClosePreviewLibrary( (PTR) IDataTypes );

	ClosePreviewLibrary( (PTR) IGadTools );

	ClosePreviewLibrary( (PTR) IGraphics );

	ClosePreviewLibrary( (PTR) IIntuition );

	free( preview.SpeedModes );

	free( preview.SpeedLabels[1] );

	free( preview.SpeedLabels[0] );

	free( preview.SizeChoices[1] );

	free( preview.SizeChoices[0] );

	free( preview.SizeLabels[1] );

	free( preview.SizeLabels[0] );

	free( preview.Sizes );

	free( preview.Modes );

	return ( result );
}

// --
