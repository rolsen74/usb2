
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <exec/errors.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <libraries/gadtools.h>
#include <classes/window.h>
#include <gadgets/button.h>
#include <gadgets/checkbox.h>
#include <gadgets/chooser.h>
#include <gadgets/getfile.h>
#include <gadgets/layout.h>
#include <gadgets/slider.h>
#include <gadgets/space.h>
#include <images/bevel.h>
#include <proto/graphics.h>
#include <proto/datatypes.h>
#include <datatypes/pictureclass.h>
#include <interfaces/timer.h>
#include <devices/usb2.h>
#include "../inc/VideoDevice.h"
#include "Preview.h"
#include "PreviewAudio.h"
#include "PreviewAvi.h"

// --

struct IntuitionIFace * IIntuition;
struct GraphicsIFace *	IGraphics;
struct DataTypesIFace * IDataTypes;

#define PREVIEW_START		  1
#define PREVIEW_SIZE		  2
#define PREVIEW_SPEED		  3
#define PREVIEW_FORMAT		  4
#define PREVIEW_PLAY		  5
#define PREVIEW_CAPTURE_AUDIO 6
#define PREVIEW_AUDIO_OUT	  7
#define PREVIEW_AUDIO_IN	  8
#define PREVIEW_CAMERA		  9
#define PREVIEW_VIDEO		  10
#define PREVIEW_RECORD_VIDEO  11
#define PREVIEW_VIDEO_FILE	  12
#define PREVIEW_AUDIO_FILE	  13
#define PREVIEW_RECORD_AUDIO  14
#define PREVIEW_VOLUME		  15
#define PREVIEW_MIC_GAIN	  16
#define PREVIEW_OUTPUT_RATE	  17
#define PREVIEW_WIDTH		  640
#define PREVIEW_HEIGHT		  480

enum SettingsClass
{
	SETTINGS_WINDOW,
	SETTINGS_LAYOUT,
	SETTINGS_CHECKBOX,
	SETTINGS_CHOOSER,
	SETTINGS_GETFILE,
	SETTINGS_BUTTON,
	SETTINGS_SLIDER,
	SETTINGS_SPACE,
	SETTINGS_CLASS_COUNT
};

enum SettingsAction
{
	SETTINGS_ERROR,
	SETTINGS_CLOSE,
	SETTINGS_START
};

static STR CurrentModeLabels[] = { "Current", NULL };

struct PreviewMode
{
	U32	 Number;
	U32	 Format;
	U32	 SizeIndex;
	U32	 Interval;
	U32	 Width;
	U32	 Height;
	char Label[24];
};

struct PreviewSize
{
	U32	 Format;
	U32	 Choice;
	U32	 Width;
	U32	 Height;
	char Label[24];
};

struct PreviewModeChoices
{
	struct PreviewMode * Modes;
	struct PreviewSize * Sizes;

	STR	  FormatLabels[3];
	U32	  Formats[2];
	U32	  FormatCount;
	STR * SizeLabels[2];
	U32 * SizeChoices[2];
	U32	  SizeCounts[2];
	U32	  DefaultModes[2];
	char  DefaultLabels[2][48];
	STR * SpeedLabels[2];
	U32 * SpeedModes;
	U32	  ModeCount;
	U32	  SizeCount;
	U32	  SpeedCount;
	U32	  SpeedActive;
	U32	  SpeedSlot;
	U32	  ActiveMode;
};

struct Preview
{
	struct Window *				Window;
	struct BitMap *				Bitmap;
	struct PreviewModeChoices * Choices;
	struct PreviewMode			ReconnectMode;
	struct Gadget *				StartGadget;
	struct Gadget *				VideoGadget;
	struct Gadget *				CameraGadget;
	struct Gadget *				FormatGadget;
	struct Gadget *				SizeGadget;
	struct Gadget *				SpeedGadget;
	struct Gadget *				PlayGadget;
	struct Gadget *				AudioCaptureGadget;
	struct Gadget *				OutputGadget;
	struct Gadget *				OutputRateGadget;
	struct Gadget *				InputGadget;
	struct Gadget *				RecordVideoGadget;
	struct Gadget *				RecordAudioGadget;
	struct Gadget *				VideoFileGadget;
	struct Gadget *				AudioFileGadget;
	struct Gadget *				MicGainGadget;
	struct PreviewAudioChoices	Inputs;
	struct PreviewAudioChoices	Outputs;
	Object *					Picture;

	STR * SizeMenuLabels;
	U8 *  RGB;
	U32	  Width;
	U32	  Height;
	U32	  MaxWidth;
	U32	  MaxHeight;
	U32	  Left;
	U32	  Top;
	char  CaptureTitle[2][96];
	U32	  TitleSlot;
	U32	  Volume;
	S32	  VideoAvailable;
	S32	  CameraLost;
	S32	  Video;
	S32	  Play;
	S32	  AudioCapture;
	S32	  RecordAudio;
	S32	  RecordVideo;
	S32	  Trace;
	char  VideoPath[PREVIEW_PATH_SIZE];
	char  AudioPath[PREVIEW_PATH_SIZE];
};

struct PreviewTiming
{
	struct TimerIFace * Clock;

	U64 Start;
	U64 PictureReady;
	U64 DrawStart;
};

struct PreviewTimeField
{
	struct Gadget * Gadget;

	char Text[2][32];
	U32	 Current;
};

static const char * const PreviewTimeLabels[] = { "Capture", "Decode", "Display" };

#define PREVIEW_TIME_FIELDS ( sizeof ( PreviewTimeLabels ) / sizeof ( PreviewTimeLabels[0] ) )

struct PreviewPixels
{
	U8 *	Data;
	U32		BytesPerRow;
	PIX_FMT Format;
};

// --

static PTR
OpenPreviewLibrary ( const char * name )
{
	struct Library *   base;
	struct Interface * iface;

	base = OpenLibrary ( name, 50 );

	if ( ! base )
	{
		return ( NULL );
	}

	iface = GetInterface ( base, "main", 1, NULL );

	if ( ! iface )
	{
		CloseLibrary ( base );
	}

	return ( iface );
}

// --

static void
ClosePreviewLibrary ( struct Interface * iface )
{
	struct Library * base;

	if ( iface )
	{
		base = iface->Data.LibBase;

		DropInterface ( iface );

		CloseLibrary ( base );
	}
}

// --

static void
ClearPreview ( const struct Preview * preview )
{
	EraseRect ( preview->Window->RPort, preview->Left, preview->Top, preview->Left + preview->MaxWidth - 1,
				preview->Top + preview->MaxHeight - 1 );
}

// --

static void
DrawPreview ( const struct Preview * preview, const struct PreviewPixels * pixels )
{
	U32 left;
	U32 top;

	left = preview->Left + ( preview->MaxWidth - preview->Width ) / 2;
	top	 = preview->Top + ( preview->MaxHeight - preview->Height ) / 2;

	if ( pixels->Data )
	{
		WritePixelArray ( pixels->Data, 0, 0, pixels->BytesPerRow, pixels->Format, preview->Window->RPort, left, top,
						  preview->Width, preview->Height );
	}
	else
	{
		BltBitMapRastPort ( preview->Bitmap, 0, 0, preview->Window->RPort, left, top, preview->Width, preview->Height, 0xc0 );
	}
}

// --

static void
ReleasePreviewPicture ( struct Preview * preview )
{
	if ( preview->Picture )
	{
		WaitBlit ();

		DisposeDTObject ( preview->Picture );

		preview->Picture = NULL;
		preview->Bitmap	 = NULL;
	}
}

// --

static S32
ObtainPreviewPixels ( Object * picture, const struct Preview * preview, struct PreviewPixels * pixels )
{
#ifdef PDTM_OBTAINPIXELARRAY
	struct pdtBlitPixelArray   array;
	struct pdtObtainPixelArray obtain;

	U32 pixelSize;

	if ( GetBitMapAttr ( preview->Window->WScreen->RastPort.BitMap, BMA_DEPTH ) <= 8 )
	{
		return ( FALSE );
	}

	array				   = (struct pdtBlitPixelArray){ 0 };
	obtain.MethodID		   = PDTM_OBTAINPIXELARRAY;
	obtain.popa_PixelArray = &array;
	obtain.popa_Flags	   = 0;

	if ( ( ! IDoMethodA ( picture, (Msg)&obtain ) ) || ( ! array.pbpa_PixelData ) || ( array.pbpa_Left ) || ( array.pbpa_Top )
		 || ( array.pbpa_Width < preview->Width ) || ( array.pbpa_Height < preview->Height ) )
	{
		return ( FALSE );
	}

	switch ( array.pbpa_PixelFormat )
	{
		case PBPAFMT_RGB:
			pixelSize	   = 3;
			pixels->Format = PIXF_R8G8B8;
			break;

		case PBPAFMT_ARGB:
			pixelSize	   = 4;
			pixels->Format = PIXF_A8R8G8B8;
			break;

		case PBPAFMT_RGBA:
			pixelSize	   = 4;
			pixels->Format = PIXF_R8G8B8A8;
			break;

		default:
			return ( FALSE );
	}

	if ( array.pbpa_PixelArrayMod < preview->Width * pixelSize )
	{
		return ( FALSE );
	}

	pixels->Data		= array.pbpa_PixelData;
	pixels->BytesPerRow = array.pbpa_PixelArrayMod;

	return ( TRUE );
#else
	(void)picture;
	(void)preview;
	(void)pixels;

	return ( FALSE );
#endif
}

// --

static S32
DecodeFrame ( struct Preview * preview, U8 * frame, U32 size, struct PreviewTiming * timing, struct PreviewPixels * pixels )
{
	struct BitMapHeader * header;
	struct BitMap *		  bitmap;
	struct gpLayout		  layout;
	Object *			  picture;

	picture = NewDTObject ( NULL, DTA_SourceType, DTST_MEMORY, DTA_SourceAddress, frame, DTA_SourceSize, size, DTA_GroupID,
							GID_PICTURE, PDTA_DestMode, PMODE_V43, PDTA_Screen, preview->Window->WScreen, PDTA_Remap, TRUE,
							PDTA_FreeSourceBitMap, TRUE, TAG_END );

	if ( ! picture )
	{
		return ( FALSE );
	}

	if ( timing->Clock )
	{
		timing->PictureReady = VideoTest_UptimeUS ( timing->Clock );
	}

	header = NULL;
	bitmap = NULL;

	GetDTAttrs ( picture, PDTA_BitMapHeader, &header, TAG_END );

	if ( ( ! header ) || ( ! header->bmh_Width ) || ( ! header->bmh_Height ) )
	{
		goto failed;
	}

	// Borrow native pixels while the picture is alive and the blit can still use them.
	if ( ( header->bmh_Width == preview->Width ) && ( header->bmh_Height == preview->Height )
		 && ( ObtainPreviewPixels ( picture, preview, pixels ) ) )
	{
		preview->Picture = picture;

		return ( TRUE );
	}

	if ( ( header->bmh_Width != preview->Width ) || ( header->bmh_Height != preview->Height ) )
	{
		struct pdtScale scale;

		scale.MethodID	   = PDTM_SCALE;
		scale.ps_NewWidth  = preview->Width;
		scale.ps_NewHeight = preview->Height;
		scale.ps_Flags	   = 0;

		if ( ! IDoMethodA ( picture, (Msg)&scale ) )
		{
			goto failed;
		}
	}

	layout.MethodID	   = DTM_PROCLAYOUT;
	layout.gpl_GInfo   = NULL;
	layout.gpl_Initial = TRUE;

	if ( ! IDoMethodA ( picture, (Msg)&layout ) )
	{
		goto failed;
	}

	GetDTAttrs ( picture, PDTA_DestBitMap, &bitmap, TAG_END );

	if ( ! bitmap )
	{
		goto failed;
	}

	preview->Picture = picture;
	preview->Bitmap	 = bitmap;

	return ( TRUE );

failed:
	DisposeDTObject ( picture );

	return ( FALSE );
}

// --

static void
FinishRequest ( struct IORequest * request )
{
	if ( ! CheckIO ( request ) )
	{
		AbortIO ( request );
	}

	WaitIO ( request );
}

// --

static S32
RenderPreviewFrame ( struct Preview * preview, const struct VideoFormat * format, U8 * frame, U32 size,
					 struct PreviewTiming * timing, S32 * direct )
{
	struct PreviewPixels pixels;

	S32 raw;

	pixels = (struct PreviewPixels){ 0 };
	raw	   = format->Format == VIDEO_FORMAT_YUY2;

	if ( ( ! size ) || ( size > format->MaxFrameSize ) )
	{
		return ( FALSE );
	}

	if ( timing->Clock )
	{
		timing->Start = VideoTest_UptimeUS ( timing->Clock );
	}

	if ( raw )
	{
		if ( ! preview->RGB )
		{
			preview->RGB = AllocVecTags ( preview->MaxWidth * preview->MaxHeight * 3, TAG_END );
		}

		if ( ( ! preview->RGB )
			 || ( ! VideoTest_YUY2ToRGB ( frame, size, format->Width, format->Height, preview->RGB, preview->Width,
										  preview->Height ) ) )
		{
			return ( FALSE );
		}

		pixels.Data		   = preview->RGB;
		pixels.BytesPerRow = preview->Width * 3;
		pixels.Format	   = PIXF_R8G8B8;
	}
	else if ( ! DecodeFrame ( preview, frame, size, timing, &pixels ) )
	{
		return ( FALSE );
	}

	if ( timing->Clock )
	{
		timing->DrawStart = VideoTest_UptimeUS ( timing->Clock );

		if ( raw )
		{
			timing->PictureReady = timing->DrawStart;
		}
	}

	*direct = ( ! raw ) && ( pixels.Data );

	DrawPreview ( preview, &pixels );

	return ( TRUE );
}

// --

static void
StartPreviewTimer ( struct TimeRequest * timer, U32 seconds )
{
	timer->Request.io_Command = TR_ADDREQUEST;
	timer->Request.io_Flags	  = 0;
	timer->Time.Seconds	  = seconds;
	timer->Time.Microseconds  = 0;

	SendIO ( &timer->Request );
}

// --

static U64
StartFrame ( struct IOStdReq * ioreq, struct TimeRequest * timer, U8 * frame, U32 size, struct TimerIFace * clock )
{
	U64 submitted;

	ioreq->io_Command = CMD_READ;
	ioreq->io_Data = frame;
	ioreq->io_Length = size;

	StartPreviewTimer ( timer, VIDEO_FRAME_TIMEOUT_SECONDS + 11 );

	submitted = clock ? VideoTest_UptimeUS ( clock ) : 0;

	SendIO ( (PTR)ioreq );

	return ( submitted );
}

// --

U64
VideoTest_UptimeUS ( struct TimerIFace * clock )
{
	struct TimeVal time;

	clock->GetUpTime ( &time );

	return ( (U64)time.Seconds * 1000000 + time.Microseconds );
}

// --

static void
ShowPreviewRate ( struct Preview * preview, const struct VideoFormat * format, U32 rate )
{
	U32 fps;
	STR title;

	fps	  = ( 100000000UL + format->FrameInterval / 2 ) / (U64)format->FrameInterval;
	title = preview->CaptureTitle[preview->TitleSlot];

	if ( rate )
	{
		snprintf ( title, sizeof ( preview->CaptureTitle[0] ), "USB2VideoTest - %lux%lu - preview %lu.%lu / camera %lu.%lu fps",
				   (unsigned long)format->Width, (unsigned long)format->Height, (unsigned long)( rate / 10 ),
				   (unsigned long)( rate % 10 ), (unsigned long)( fps / 10 ), (unsigned long)( fps % 10 ) );
	}
	else
	{
		snprintf ( title, sizeof ( preview->CaptureTitle[0] ), "USB2VideoTest - %lux%lu - camera %lu.%lu fps",
				   (unsigned long)format->Width, (unsigned long)format->Height, (unsigned long)( fps / 10 ),
				   (unsigned long)( fps % 10 ) );
	}

	SetWindowTitles ( preview->Window, title, (CONST_STRPTR)-1 );

	preview->TitleSlot ^= 1;
}

// --

static void
ShowPreviewTimings ( struct Window * window, struct PreviewTimeField * fields, U32 frames, U64 capture, U64 decode, U64 show )
{
	struct PreviewTimeField * field;

	U64 totals[PREVIEW_TIME_FIELDS];
	U64 hundredths;
	U32 index;
	U32 next;

	totals[0] = capture;
	totals[1] = decode;
	totals[2] = show;

	for ( index = 0; index < PREVIEW_TIME_FIELDS; index++ )
	{
		field	   = &fields[index];
		next	   = field->Current ^ 1;
		if ( frames )
		{
			hundredths = ( totals[index] / frames + 5 ) / 10;

			snprintf ( field->Text[next], sizeof ( field->Text[0] ), "%s: %lu.%02lu ms", PreviewTimeLabels[index],
				(unsigned long)( hundredths / 100 ), (unsigned long)( hundredths % 100 ) );
		}
		else
		{
			snprintf ( field->Text[next], sizeof ( field->Text[0] ), "%s: -- ms", PreviewTimeLabels[index] );
		}

		if ( strcmp ( field->Text[field->Current], field->Text[next] ) )
		{
			field->Current = next;

			RefreshSetGadgetAttrs ( field->Gadget, window, NULL, GA_Text, field->Text[next], TAG_END );
		}
	}
}

// --

static void
SizePreview ( struct Preview * preview, const struct VideoFormat * format )
{
	preview->Width	= MIN ( preview->MaxWidth, format->Width );
	preview->Height = MAX ( 1, (U64)preview->Width * format->Height / format->Width );

	if ( preview->Height > preview->MaxHeight )
	{
		preview->Height = preview->MaxHeight;
		preview->Width	= MAX ( 1, (U64)preview->Height * format->Width / format->Height );
	}
}

// --

static int
ComparePreviewModes ( const void * first, const void * second )
{
	const struct PreviewMode * left;
	const struct PreviewMode * right;

	left  = first;
	right = second;

	if ( left->Format != right->Format )
	{
		return ( left->Format < right->Format ? -1 : 1 );
	}

	if ( left->Width != right->Width )
	{
		return ( left->Width < right->Width ? -1 : 1 );
	}

	if ( left->Height != right->Height )
	{
		return ( left->Height < right->Height ? -1 : 1 );
	}

	// Longer intervals are lower frame rates.
	return ( ( left->Interval < right->Interval ) - ( left->Interval > right->Interval ) );
}

// --

static S32
LoadPreviewModes ( struct PreviewModeChoices * choices, struct IOStdReq * ioreq, const struct VideoFormat * active )
{
	struct VideoFormat	 format;
	struct VideoFormat	 preferred;
	struct PreviewMode * modes;
	struct PreviewSize * entry;

	U32 index;
	U32 number;
	U32 capacity;
	U32 fps;
	U32 size;
	U32 group;
	U32 distance;
	U32 best;
	U32 defaultBest;
	U32 defaultMode;
	U32 first[2];
	U32 choice;
	S32 error;

	capacity = 0;

	choices->FormatLabels[0] = ( active->Format == VIDEO_FORMAT_YUY2 ) ? "YUY2" : "MJPEG";

	for ( number = 1;; number++ )
	{
		ioreq->io_Command = VIDEO_CMD_GET_MODE;
		ioreq->io_Data	  = &format;
		ioreq->io_Length  = sizeof ( format );
		ioreq->io_Offset  = number;

		error = DoIO ( (PTR)ioreq );

		if ( ( error == IOERR_BADADDRESS ) || ( error == IOERR_NOCMD ) )
		{
			break;
		}

		if ( error )
		{
			return ( FALSE );
		}

		if ( ( ( format.Format != VIDEO_FORMAT_MJPEG ) && ( format.Format != VIDEO_FORMAT_YUY2 ) ) || ( ! format.Width )
			 || ( ! format.Height ) || ( ! format.FrameInterval )
			 || ( ( format.Format == VIDEO_FORMAT_YUY2 ) && ( format.Width & 1 ) ) )
		{
			continue;
		}

		for ( index = 0; index < choices->ModeCount; index++ )
		{
			if ( ( choices->Modes[index].Format == format.Format ) && ( choices->Modes[index].Width == format.Width )
				 && ( choices->Modes[index].Height == format.Height )
				 && ( choices->Modes[index].Interval == format.FrameInterval ) )
			{
				break;
			}
		}

		if ( index < choices->ModeCount )
		{
			continue;
		}

		if ( choices->ModeCount == capacity )
		{
			capacity = capacity ? capacity * 2 : 16;

			modes = realloc ( choices->Modes, capacity * sizeof ( *modes ) );

			if ( ! modes )
			{
				return ( FALSE );
			}

			choices->Modes = modes;
		}

		index = choices->ModeCount++;

		choices->Modes[index].Number   = number;
		choices->Modes[index].Format   = format.Format;
		choices->Modes[index].Width	   = format.Width;
		choices->Modes[index].Height   = format.Height;
		choices->Modes[index].Interval = format.FrameInterval;

		fps = ( 100000000UL + format.FrameInterval / 2 ) / (U64)format.FrameInterval;

		snprintf ( choices->Modes[index].Label, sizeof ( choices->Modes[index].Label ), "%lu.%lu fps",
				   (unsigned long)( fps / 10 ), (unsigned long)( fps % 10 ) );
	}

	if ( ! choices->ModeCount )
	{
		return ( TRUE );
	}

	preferred = *active;

	ioreq->io_Command = VIDEO_CMD_GET_MODE;
	ioreq->io_Data	  = &format;
	ioreq->io_Length  = sizeof ( format );
	ioreq->io_Offset  = 0;

	if ( ! DoIO ( (PTR)ioreq ) )
	{
		preferred = format;
	}

	qsort ( choices->Modes, choices->ModeCount, sizeof ( *choices->Modes ), ComparePreviewModes );

	choices->Sizes			= calloc ( choices->ModeCount + 0, sizeof ( *choices->Sizes ) );
	choices->SizeLabels[0]	= calloc ( choices->ModeCount + 3, sizeof ( STR ) );
	choices->SizeLabels[1]	= calloc ( choices->ModeCount + 3, sizeof ( STR ) );
	choices->SizeChoices[0] = calloc ( choices->ModeCount + 2, sizeof ( U32 ) );
	choices->SizeChoices[1] = calloc ( choices->ModeCount + 2, sizeof ( U32 ) );
	choices->SpeedLabels[0] = calloc ( choices->ModeCount + 1, sizeof ( STR ) );
	choices->SpeedLabels[1] = calloc ( choices->ModeCount + 1, sizeof ( STR ) );
	choices->SpeedModes		= calloc ( choices->ModeCount + 0, sizeof ( *choices->SpeedModes ) );

	if ( ( ! choices->Sizes )			  //
		 || ( ! choices->SizeLabels[0] )  //
		 || ( ! choices->SizeLabels[1] )  //
		 || ( ! choices->SizeChoices[0] ) //
		 || ( ! choices->SizeChoices[1] ) //
		 || ( ! choices->SpeedLabels[0] ) //
		 || ( ! choices->SpeedLabels[1] ) //
		 || ( ! choices->SpeedModes ) )
	{
		return ( FALSE );
	}

	best		= ~0U;
	defaultBest = ~0U;
	defaultMode = 0;
	first[0]	= ~0U;
	first[1]	= ~0U;

	for ( index = 0; index < choices->ModeCount; index++ )
	{
		modes = &choices->Modes[index];
		group = modes->Format == VIDEO_FORMAT_YUY2;

		for ( size = 0; size < choices->SizeCount; size++ )
		{
			if ( ( choices->Sizes[size].Format == modes->Format ) && ( choices->Sizes[size].Width == modes->Width )
				 && ( choices->Sizes[size].Height == modes->Height ) )
			{
				break;
			}
		}

		if ( size == choices->SizeCount )
		{
			choices->Sizes[size].Format = modes->Format;
			choices->Sizes[size].Width	= modes->Width;
			choices->Sizes[size].Height = modes->Height;
			choices->SizeCount++;

			snprintf ( choices->Sizes[size].Label, sizeof ( choices->Sizes[size].Label ), "%lux%lu", (unsigned long)modes->Width,
					   (unsigned long)modes->Height );
		}

		modes->SizeIndex = size;

		if ( modes->Number < first[group] )
		{
			first[group]				 = modes->Number;
			choices->DefaultModes[group] = index;
		}

		if ( ( active->Format == modes->Format ) && ( active->Width == modes->Width ) && ( active->Height == modes->Height ) )
		{
			distance = MAX ( active->FrameInterval, modes->Interval ) - MIN ( active->FrameInterval, modes->Interval );

			if ( distance < best )
			{
				best				= distance;
				choices->ActiveMode = index;
			}
		}

		if ( ( preferred.Format == modes->Format ) && ( preferred.Width == modes->Width )
			 && ( preferred.Height == modes->Height ) )
		{
			distance = MAX ( preferred.FrameInterval, modes->Interval ) - MIN ( preferred.FrameInterval, modes->Interval );

			if ( distance < defaultBest )
			{
				defaultBest = distance;
				defaultMode = index;
			}
		}
	}

	if ( defaultBest != ~0U )
	{
		group						 = preferred.Format == VIDEO_FORMAT_YUY2;
		choices->DefaultModes[group] = defaultMode;
	}

	for ( group = 0; group < 2; group++ )
	{
		if ( first[group] == ~0U )
		{
			continue;
		}

		modes = &choices->Modes[choices->DefaultModes[group]];
		size  = modes->SizeIndex;

		choices->Formats[choices->FormatCount]		  = modes->Format;
		choices->FormatLabels[choices->FormatCount++] = group ? "YUY2" : "MJPEG";
		choices->SizeChoices[group][0]				  = size;
		choices->SizeLabels[group][0]				  = choices->DefaultLabels[group];
		choices->SizeCounts[group]					  = 1;

		snprintf ( choices->DefaultLabels[group], sizeof ( choices->DefaultLabels[group] ), "Default (%s)",
				   choices->Sizes[size].Label );
	}

	for ( size = 0; size < choices->SizeCount; size++ )
	{
		entry = &choices->Sizes[size];
		group = entry->Format == VIDEO_FORMAT_YUY2;

		if ( size == choices->SizeChoices[group][0] )
		{
			continue;
		}

		if ( choices->SizeCounts[group] == 1 )
		{
			// A chooser label of -1 is a non-selectable separator.
			choices->SizeLabels[group][choices->SizeCounts[group]++] = (STR)-1;
		}

		choice								= choices->SizeCounts[group]++;
		entry->Choice						= choice;
		choices->SizeLabels[group][choice]	= entry->Label;
		choices->SizeChoices[group][choice] = size;
	}

	return ( TRUE );
}

// --

static void
FreePreviewModes ( struct PreviewModeChoices * choices )
{
	if ( ! choices )
	{
		return;
	}

	free ( choices->SpeedModes );

	free ( choices->SpeedLabels[1] );

	free ( choices->SpeedLabels[0] );

	free ( choices->SizeChoices[1] );

	free ( choices->SizeChoices[0] );

	free ( choices->SizeLabels[1] );

	free ( choices->SizeLabels[0] );

	free ( choices->Sizes );

	free ( choices->Modes );

	free ( choices );
}

// --

static void
UpdateSpeedChoices ( struct Preview * preview )
{
	U32 index;
	U32 size;

	size						  = preview->Choices->Modes[preview->Choices->ActiveMode].SizeIndex;
	preview->Choices->SpeedCount  = 0;
	preview->Choices->SpeedActive = 0;
	preview->Choices->SpeedSlot ^= 1;

	for ( index = 0; index < preview->Choices->ModeCount; index++ )
	{
		if ( preview->Choices->Modes[index].SizeIndex != size )
		{
			continue;
		}

		if ( index == preview->Choices->ActiveMode )
		{
			preview->Choices->SpeedActive = preview->Choices->SpeedCount;
		}

		preview->Choices->SpeedLabels[preview->Choices->SpeedSlot][preview->Choices->SpeedCount]
			= preview->Choices->Modes[index].Label;
		preview->Choices->SpeedModes[preview->Choices->SpeedCount++] = index;
	}

	preview->Choices->SpeedLabels[preview->Choices->SpeedSlot][preview->Choices->SpeedCount] = NULL;
}

// --

static U32
ClosestSizeMode ( const struct Preview * preview, U32 size, U32 interval )
{
	U32 index;
	U32 selected;
	U32 best;
	U32 distance;
	U32 candidate;

	selected = preview->Choices->ActiveMode;
	best	 = ~0U;

	for ( index = 0; index < preview->Choices->ModeCount; index++ )
	{
		if ( preview->Choices->Modes[index].SizeIndex == size )
		{
			candidate = preview->Choices->Modes[index].Interval;
			distance  = MAX ( interval, candidate ) - MIN ( interval, candidate );

			if ( distance < best )
			{
				best	 = distance;
				selected = index;
			}
		}
	}

	return ( selected );
}

// --

static U32
ClosestFormatMode ( const struct Preview * preview, const struct VideoFormat * current, U32 format )
{
	const struct PreviewSize * size;

	U32 index;
	U32 selected;
	U32 distance;
	U32 best;
	U32 group;

	group	 = format == VIDEO_FORMAT_YUY2;
	selected = preview->Choices->SizeChoices[group][0];
	best	 = ~0U;

	for ( index = 0; index < preview->Choices->SizeCount; index++ )
	{
		size = &preview->Choices->Sizes[index];

		if ( size->Format != format )
		{
			continue;
		}

		distance = MAX ( size->Width, current->Width ) - MIN ( size->Width, current->Width )
				   + MAX ( size->Height, current->Height ) - MIN ( size->Height, current->Height );

		if ( distance < best )
		{
			best	 = distance;
			selected = index;
		}
	}

	return ( ClosestSizeMode ( preview, selected, current->FrameInterval ) );
}

// --

static void
RefreshModeChoices ( struct Preview * preview )
{
	const struct PreviewSize *	size;
	struct PreviewModeChoices * choices;

	STR * labels;
	U32	  count;
	U32	  selected;
	U32	  group;
	U32	  formatChoice;

	choices = preview->Choices;

	if ( ! choices->ModeCount )
	{
		labels		 = CurrentModeLabels;
		count		 = 1;
		selected	 = 0;
		formatChoice = 0;
	}
	else
	{
		size		 = &choices->Sizes[choices->Modes[choices->ActiveMode].SizeIndex];
		group		 = size->Format == VIDEO_FORMAT_YUY2;
		labels		 = choices->SizeLabels[group];
		count		 = choices->SizeCounts[group];
		selected	 = size->Choice;
		formatChoice = choices->Formats[0] != size->Format;

		UpdateSpeedChoices ( preview );
	}

	RefreshSetGadgetAttrs ( preview->FormatGadget, preview->Window, NULL, CHOOSER_LabelArray, choices->FormatLabels,
							CHOOSER_Selected, formatChoice, TAG_END );

	// Rebind the size list only when the camera or pixel format changes.
	if ( labels != preview->SizeMenuLabels )
	{
		RefreshSetGadgetAttrs ( preview->SizeGadget, preview->Window, NULL, CHOOSER_MaxLabels, count, CHOOSER_LabelArray, labels,
								CHOOSER_Selected, selected, TAG_END );

		preview->SizeMenuLabels = labels;
	}
	else
	{
		RefreshSetGadgetAttrs ( preview->SizeGadget, preview->Window, NULL, CHOOSER_Selected, selected, TAG_END );
	}

	RefreshSetGadgetAttrs ( preview->SpeedGadget, preview->Window, NULL, CHOOSER_LabelArray,
							choices->ModeCount ? choices->SpeedLabels[choices->SpeedSlot] : CurrentModeLabels, CHOOSER_MaxLabels,
							MAX ( 1, choices->SpeedCount ), CHOOSER_Selected, choices->SpeedActive, TAG_END );
}

// --

static S32
SetCameraMode ( struct IOStdReq * ioreq, U32 number )
{
	ioreq->io_Command = VIDEO_CMD_SET_MODE;
	ioreq->io_Offset = number;
	ioreq->io_Data = NULL;
	ioreq->io_Length = 0;

	return ( DoIO ( (PTR)ioreq ) );
}

// --

static S32
ChangePreviewMode ( struct Preview * preview, struct IOStdReq * ioreq, struct VideoFormat * format, U32 index )
{
	struct VideoFormat		   actual;
	const struct PreviewSize * size;
	const struct PreviewMode * mode;

	U32 found;
	S32 error;
	S32 query;

	mode = &preview->Choices->Modes[index];

	error = SetCameraMode ( ioreq, mode->Number );

	query = VideoTest_QueryCamera ( ioreq, &actual );

	if ( ( query == USB2Err_Device_Detached ) || ( ( query == IOERR_UNITBUSY ) && ( error == IOERR_UNITBUSY ) ) )
	{
		preview->CameraLost	   = TRUE;
		preview->ReconnectMode = *mode;

		Printf ( "Camera %04lx:%04lx is unavailable. Reconnect it and select a camera or mode again.\n", format->VendorID,
				 format->DeviceID );

		SetWindowTitles ( preview->Window, "USB2VideoTest - Camera unavailable", (CONST_STRPTR)-1 );

		return ( FALSE );
	}

	if ( ( query ) || ( ( actual.Format != VIDEO_FORMAT_MJPEG ) && ( actual.Format != VIDEO_FORMAT_YUY2 ) ) || ( ! actual.Width )
		 || ( ! actual.Height ) || ( ! actual.FrameInterval ) || ( ! actual.MaxFrameSize ) )
	{
		format->MaxFrameSize = 0;
	}
	else
	{
		*format = actual;

		for ( found = 0; found < preview->Choices->SizeCount; found++ )
		{
			size = &preview->Choices->Sizes[found];

			if ( ( size->Format == actual.Format ) && ( size->Width == actual.Width ) && ( size->Height == actual.Height ) )
			{
				preview->Choices->ActiveMode = ClosestSizeMode ( preview, found, actual.FrameInterval );
				break;
			}
		}
	}

	if ( ( error ) || ( ! format->MaxFrameSize ) )
	{
		Printf ( "Could not set %lu x %lu, camera mode %lu (set error %ld, query error %ld).\n", mode->Width, mode->Height,
				 mode->Number, error, query );

		SetWindowTitles ( preview->Window, "USB2VideoTest - Could not set camera mode", (CONST_STRPTR)-1 );

		return ( FALSE );
	}

	return ( TRUE );
}

// --

static S32
PrepareFrameBuffers ( U8 ** frame, U8 ** spare, U32 * capacity, U32 size )
{
	if ( ! size )
	{
		return ( FALSE );
	}

	if ( size > *capacity )
	{
		FreeVec ( *frame );

		FreeVec ( *spare );

		*frame = AllocVecTags ( size, TAG_END );

		*spare = AllocVecTags ( size, TAG_END );

		*capacity = ( ( *frame ) && ( *spare ) ) ? size : 0;
	}

	return ( *capacity != 0 );
}

// --

static S32
CanStart ( const struct Preview * preview, const struct VideoFormat * format )
{
	return ( ( ( ! preview->Video ) || ( ( ! preview->CameraLost ) && ( format->MaxFrameSize ) ) )
			 && ( ( preview->Video ) || ( ( preview->AudioCapture ) && ( ( preview->RecordAudio ) || ( preview->Play ) ) ) ) );
}

// --

static void
UpdateSettingsControls ( struct Preview * preview, const struct VideoFormat * format, const struct PreviewCameras * cameras )
{
	RefreshSetGadgetAttrs (
		preview->StartGadget, preview->Window, NULL, GA_Disabled,
		( ! CanStart ( preview, format ) ) && ( ! ( preview->Video && preview->CameraLost && cameras->Count ) ), TAG_END );

	RefreshSetGadgetAttrs ( preview->VideoGadget, preview->Window, NULL, CHECKBOX_Checked, preview->Video, GA_Disabled,
							! preview->VideoAvailable, TAG_END );

	RefreshSetGadgetAttrs ( preview->CameraGadget, preview->Window, NULL, GA_Disabled, ( ! preview->Video || ! cameras->Count ),
							TAG_END );

	if ( preview->SizeGadget )
	{
		RefreshSetGadgetAttrs ( preview->FormatGadget, preview->Window, NULL, GA_Disabled, ! preview->Video, TAG_END );

		RefreshSetGadgetAttrs ( preview->SizeGadget, preview->Window, NULL, GA_Disabled,
								( ! preview->Video || ! preview->Choices->ModeCount ), TAG_END );

		RefreshSetGadgetAttrs ( preview->SpeedGadget, preview->Window, NULL, GA_Disabled,
								( ! preview->Video || ! preview->Choices->ModeCount ), TAG_END );
	}

	RefreshSetGadgetAttrs ( preview->PlayGadget, preview->Window, NULL, CHECKBOX_Checked, preview->Play, GA_Disabled,
							( ! preview->AudioCapture || ! preview->Outputs.Count ), TAG_END );

	RefreshSetGadgetAttrs ( preview->AudioCaptureGadget, preview->Window, NULL, CHECKBOX_Checked, preview->AudioCapture,
							GA_Disabled, ! preview->Inputs.Count, TAG_END );

	RefreshSetGadgetAttrs ( preview->OutputGadget, preview->Window, NULL, GA_Disabled,
							( ! preview->AudioCapture || ! preview->Play || ! preview->Outputs.Count ), TAG_END );

	RefreshSetGadgetAttrs (
		preview->OutputRateGadget, preview->Window, NULL, GA_Disabled,
		( ! preview->AudioCapture || ! preview->Play || preview->Outputs.Rates[preview->Outputs.Selected].Count <= 1 ),
		TAG_END );

	RefreshSetGadgetAttrs ( preview->InputGadget, preview->Window, NULL, GA_Disabled, ! preview->AudioCapture, TAG_END );

	RefreshSetGadgetAttrs ( preview->MicGainGadget, preview->Window, NULL, SLIDER_Level,
							preview->Inputs.GainLevels[preview->Inputs.Selected], GA_Disabled,
							( ! preview->AudioCapture || ! preview->Inputs.Gains[preview->Inputs.Selected].Step ), TAG_END );

	if ( preview->RecordVideoGadget )
	{
		RefreshSetGadgetAttrs ( preview->RecordVideoGadget, preview->Window, NULL, CHECKBOX_Checked, preview->RecordVideo,
								GA_Disabled, ! preview->Video, TAG_END );

		RefreshSetGadgetAttrs ( preview->VideoFileGadget, preview->Window, NULL, GA_Disabled,
								( ! preview->Video || ! preview->RecordVideo ), TAG_END );
	}

	RefreshSetGadgetAttrs ( preview->RecordAudioGadget, preview->Window, NULL, CHECKBOX_Checked, preview->RecordAudio,
							GA_Disabled, ! preview->AudioCapture, TAG_END );

	RefreshSetGadgetAttrs ( preview->AudioFileGadget, preview->Window, NULL, GA_Disabled,
							( ! preview->AudioCapture || ! preview->RecordAudio ), TAG_END );
}

// --

static void
CopyRecordingDrawer ( char * drawer, const char * path )
{
	const char * file;

	file = FilePart ( path );

	memcpy ( drawer, path, file - path );

	drawer[file - path] = 0;
}

// --

static S32
UseRecordingPath ( char * path, const char * drawer, const char * file )
{
	char selected[PREVIEW_PATH_SIZE];
	BPTR lock;
	S32	 valid;

	if ( ! file[0] )
	{
		return ( FALSE );
	}

	lock = Lock ( drawer, ACCESS_READ );

	if ( ! lock )
	{
		return ( FALSE );
	}

	valid = NameFromLock ( lock, selected, sizeof ( selected ) ) && AddPart ( selected, file, sizeof ( selected ) );

	UnLock ( lock );

	if ( valid )
	{
		strcpy ( path, selected );
	}

	return ( valid );
}

// --

static void
ChooseRecordingFile ( struct Window * window, struct Gadget * gadget, char * path )
{
	ULONG drawer;
	ULONG file;

	if ( ! gfRequestFile ( (Object *)gadget, window ) )
	{
		return;
	}

	if ( ( GetAttr ( GETFILE_Drawer, (Object *)gadget, &drawer ) ) && ( drawer )
		 && ( GetAttr ( GETFILE_File, (Object *)gadget, &file ) ) && ( file )
		 && ( UseRecordingPath ( path, (const char *)drawer, (const char *)file ) ) )
	{
		SetWindowTitles ( window, "USB2VideoTest - Settings", (CONST_STRPTR)-1 );
	}
	else
	{
		SetWindowTitles ( window, "USB2VideoTest - Could not use the selected file", (CONST_STRPTR)-1 );
	}

	RefreshSetGadgetAttrs ( gadget, window, NULL, GETFILE_FullFile, path, TAG_END );
}

// --

static S32
ConfirmRecordingFile ( struct Window * window, const char * path )
{
	struct EasyStruct confirm;

	BPTR lock;

	lock = Lock ( path, ACCESS_READ );

	if ( ! lock )
	{
		return ( TRUE );
	}

	UnLock ( lock );

	confirm = (struct EasyStruct){ .es_StructSize	= sizeof ( confirm ),
								   .es_Title		= "USB2VideoTest",
								   .es_TextFormat	= "%s already exists. Keep it and save the new recording separately?",
								   .es_GadgetFormat = "Keep both|Cancel" };

	return ( EasyRequest ( window, &confirm, NULL, path ) != 0 );
}

// --

static S32
SameRecordingFile ( const char * video, const char * audio )
{
	char videoDrawer[PREVIEW_PATH_SIZE];
	char audioDrawer[PREVIEW_PATH_SIZE];
	BPTR videoLock;
	BPTR audioLock;
	S32	 same;

	if ( ! strcasecmp ( video, audio ) )
	{
		return ( TRUE );
	}

	// Compare parent drawers for files that have yet to be created.
	if ( ! strcasecmp ( FilePart ( video ), FilePart ( audio ) ) )
	{
		CopyRecordingDrawer ( videoDrawer, video );

		CopyRecordingDrawer ( audioDrawer, audio );

		video = videoDrawer;
		audio = audioDrawer;
	}

	videoLock = Lock ( video, ACCESS_READ );

	audioLock = Lock ( audio, ACCESS_READ );

	same = ( videoLock ) && ( audioLock ) && ( SameLock ( videoLock, audioLock ) == LOCK_SAME );

	if ( audioLock )
	{
		UnLock ( audioLock );
	}

	if ( videoLock )
	{
		UnLock ( videoLock );
	}

	return ( same );
}

// --

static S32
ConfirmRecordingFiles ( const struct Preview * preview )
{
	S32 video;
	S32 audio;

	video = preview->Video && preview->RecordVideo;
	audio = preview->AudioCapture && preview->RecordAudio;

	if ( ( video ) && ( audio ) && ( SameRecordingFile ( preview->VideoPath, preview->AudioPath ) ) )
	{
		SetWindowTitles ( preview->Window, "USB2VideoTest - Choose different AVI and WAV files", (CONST_STRPTR)-1 );

		return ( FALSE );
	}

	return ( ( ( ! video ) || ( ConfirmRecordingFile ( preview->Window, preview->VideoPath ) ) )
			 && ( ( ! audio ) || ( ConfirmRecordingFile ( preview->Window, preview->AudioPath ) ) ) );
}

// --

static S32
CanPreviewFormat ( const struct VideoFormat * format )
{
	return ( ( ( format->Format == VIDEO_FORMAT_MJPEG ) || ( format->Format == VIDEO_FORMAT_YUY2 ) ) && ( format->Width )
			 && ( format->Height ) && ( format->MaxFrameSize ) && ( format->FrameInterval ) );
}

// --

static void
ChangeCamera ( struct Preview * preview, struct IOStdReq ** ioreq, struct VideoFormat * format, struct PreviewCameras * cameras,
			   U32 index )
{
	struct IOStdReq *			request;
	struct VideoFormat			nextFormat;
	struct VideoFormat			previous;
	struct PreviewModeChoices * choices;
	struct PreviewModeChoices * oldChoices;
	const struct PreviewSize *	size;

	U32 candidate;
	U32 indexSize;
	S32 opened;
	S32 reconnect;

	opened	  = FALSE;
	reconnect = preview->CameraLost && ( index == cameras->Selected );
	previous  = *format;

	if ( ( reconnect ) && ( preview->ReconnectMode.Width ) )
	{
		previous.Format		   = preview->ReconnectMode.Format;
		previous.Width		   = preview->ReconnectMode.Width;
		previous.Height		   = preview->ReconnectMode.Height;
		previous.FrameInterval = preview->ReconnectMode.Interval;
	}

	choices = calloc ( 1, sizeof ( *choices ) );
	request = *ioreq;

	if ( ( choices ) && ( reconnect ) )
	{
		opened = ! VideoTest_QueryCamera ( request, &nextFormat );
	}

	if ( ( choices ) && ( ! opened ) && ( ( ! reconnect ) ||
		( ! LIB_IS_AT_LEAST ( (struct Library *)request->io_Device, VIDEO_DEVICE_VERSION, VIDEO_RECONNECT_REVISION ) ) ) )
	{
		request = (PTR)AllocSysObjectTags ( ASOT_IOREQUEST, ASOIOR_Size, sizeof ( *request ), ASOIOR_ReplyPort,
			( *ioreq )->io_Message.mn_ReplyPort, TAG_END );

		if ( request )
		{
			opened = VideoTest_OpenCamera ( request, &nextFormat, cameras->Units[index] );
		}
	}

	if ( ( opened ) && ( CanPreviewFormat ( &nextFormat ) )
		 && ( ( ! reconnect ) || ( ( nextFormat.VendorID == format->VendorID ) && ( nextFormat.DeviceID == format->DeviceID ) ) )
		 && ( LoadPreviewModes ( choices, request, &nextFormat ) ) )
	{
		if ( request != *ioreq )
		{
			CloseDevice ( (PTR)*ioreq );

			FreeSysObject ( ASOT_IOREQUEST, *ioreq );

			*ioreq = request;
		}

		*format						 = nextFormat;
		preview->CameraLost			 = FALSE;
		preview->ReconnectMode.Width = 0;
		cameras->Selected			 = index;
		oldChoices					 = preview->Choices;
		preview->Choices			 = choices;

		if ( reconnect )
		{
			for ( indexSize = 0; indexSize < choices->SizeCount; indexSize++ )
			{
				size = &choices->Sizes[indexSize];

				if ( ( size->Format == previous.Format ) && ( size->Width == previous.Width )
					 && ( size->Height == previous.Height ) )
				{
					candidate = ClosestSizeMode ( preview, indexSize, previous.FrameInterval );

					if ( candidate != choices->ActiveMode )
					{
						ChangePreviewMode ( preview, request, format, candidate );
					}

					break;
				}
			}
		}

		RefreshModeChoices ( preview );

		FreePreviewModes ( oldChoices );

		PreviewAudio_SelectInput ( &preview->Inputs, format->VendorID, format->DeviceID );

		RefreshSetGadgetAttrs ( preview->InputGadget, preview->Window, NULL, CHOOSER_Selected, preview->Inputs.Selected,
								TAG_END );

		UpdateSettingsControls ( preview, format, cameras );

		if ( ( ! preview->CameraLost ) && ( format->MaxFrameSize ) )
		{
			SetWindowTitles ( preview->Window, "USB2VideoTest - Settings", (CONST_STRPTR)-1 );
		}

		return;
	}

	if ( ( opened ) && ( request != *ioreq ) )
	{
		CloseDevice ( (PTR)request );
	}

	if ( ( request ) && ( request != *ioreq ) )
	{
		FreeSysObject ( ASOT_IOREQUEST, request );
	}

	FreePreviewModes ( choices );

	RefreshSetGadgetAttrs ( preview->CameraGadget, preview->Window, NULL, CHOOSER_Selected, cameras->Selected, TAG_END );

	SetWindowTitles ( preview->Window, reconnect ? "USB2VideoTest - Camera unavailable" :
		"USB2VideoTest - Could not switch camera", (CONST_STRPTR)-1 );
}

// --

static U32
SettingsSelection ( const struct Preview * preview, U32 gadget )
{
	struct Gadget * chooser;

	ULONG selected;

	switch ( gadget )
	{
		case PREVIEW_CAMERA:
			chooser = preview->CameraGadget;
			break;
		case PREVIEW_FORMAT:
			chooser = preview->FormatGadget;
			break;
		case PREVIEW_SIZE:
			chooser = preview->SizeGadget;
			break;
		case PREVIEW_SPEED:
			chooser = preview->SpeedGadget;
			break;
		case PREVIEW_AUDIO_IN:
			chooser = preview->InputGadget;
			break;
		case PREVIEW_AUDIO_OUT:
			chooser = preview->OutputGadget;
			break;
		case PREVIEW_OUTPUT_RATE:
			chooser = preview->OutputRateGadget;
			break;
		default:
			chooser = NULL;
			break;
	}

	selected = ~0UL;

	if ( chooser )
	{
		GetAttr ( CHOOSER_Selected, (Object *)chooser, &selected );
	}

	return ( selected );
}

// --

static U32
ReadLevelSlider ( struct Gadget * gadget, U32 current )
{
	ULONG level;

	if ( GetAttr ( SLIDER_Level, (Object *)gadget, &level ) )
	{
		return ( MIN ( 100, level ) );
	}

	return ( current );
}

// --

static enum SettingsAction
RunSettings ( struct Preview * preview, Object * settings, struct IOStdReq ** ioreq, struct VideoFormat * format,
			  struct PreviewCameras * cameras )
{
	struct Window *			   window;
	const struct PreviewMode * requested;

	U32	  code;
	U32	  event;
	U32	  gadget;
	U32	  group;
	ULONG mask;
	U32	  mode;
	U32	  level;
	U16	  input;

	window = preview->Window;
	mask   = 0;

	if ( ( ! GetAttr ( WINDOW_SigMask, settings, &mask ) ) || ( ! mask ) )
	{
		return ( SETTINGS_ERROR );
	}

	mask |= SIGBREAKF_CTRL_C;

	while ( TRUE )
	{
		if ( Wait ( mask ) & SIGBREAKF_CTRL_C )
		{
			return ( SETTINGS_CLOSE );
		}

		while ( ( event = IDoMethod ( settings, WM_HANDLEINPUT, &input ) ) != WMHI_LASTMSG )
		{
			if ( ( event & WMHI_CLASSMASK ) == WMHI_CLOSEWINDOW )
			{
				return ( SETTINGS_CLOSE );
			}

			if ( ( event & WMHI_CLASSMASK ) != WMHI_GADGETUP )
			{
				continue;
			}

			gadget = event & WMHI_GADGETMASK;

			code = SettingsSelection ( preview, gadget );

			if ( ( gadget == PREVIEW_CAMERA ) && ( preview->Video ) && ( code < cameras->Count )
				 && ( ( code != cameras->Selected ) || ( preview->CameraLost ) ) )
			{
				ChangeCamera ( preview, ioreq, format, cameras, code );
			}
			else if ( ( preview->Video ) && ( preview->Choices->ModeCount )
					  && ( ( gadget == PREVIEW_FORMAT ) || ( gadget == PREVIEW_SIZE ) || ( gadget == PREVIEW_SPEED ) ) )
			{
				group = preview->Choices->Modes[preview->Choices->ActiveMode].Format == VIDEO_FORMAT_YUY2;

				if ( ( gadget == PREVIEW_FORMAT ) && ( code < preview->Choices->FormatCount ) )
				{
					mode = ClosestFormatMode ( preview, format, preview->Choices->Formats[code] );
				}
				else if ( ( gadget == PREVIEW_SIZE ) && ( ! code ) )
				{
					mode = preview->Choices->DefaultModes[group];
				}
				else if ( ( gadget == PREVIEW_SIZE ) && ( code > 1 ) && ( code < preview->Choices->SizeCounts[group] ) )
				{
					mode = ClosestSizeMode ( preview, preview->Choices->SizeChoices[group][code], format->FrameInterval );
				}
				else if ( ( gadget == PREVIEW_SPEED ) && ( code < preview->Choices->SpeedCount ) )
				{
					mode = preview->Choices->SpeedModes[code];
				}
				else
				{
					continue;
				}

				requested = &preview->Choices->Modes[mode];

				if ( preview->CameraLost )
				{
					preview->ReconnectMode = *requested;

					ChangeCamera ( preview, ioreq, format, cameras, cameras->Selected );

					continue;
				}

				if ( ( format->MaxFrameSize ) && ( requested->Format == format->Format ) && ( requested->Width == format->Width )
					 && ( requested->Height == format->Height ) && ( requested->Interval == format->FrameInterval ) )
				{
					continue;
				}

				if ( ChangePreviewMode ( preview, *ioreq, format, mode ) )
				{
					SetWindowTitles ( window, "USB2VideoTest - Settings", (CONST_STRPTR)-1 );
				}

				RefreshModeChoices ( preview );

				UpdateSettingsControls ( preview, format, cameras );
			}
			else if ( ( gadget == PREVIEW_VIDEO ) && ( preview->VideoAvailable ) )
			{
				preview->Video = ! preview->Video;

				UpdateSettingsControls ( preview, format, cameras );
			}
			else if ( ( gadget == PREVIEW_PLAY ) && ( preview->AudioCapture ) && ( preview->Outputs.Count ) )
			{
				preview->Play = ! preview->Play;

				UpdateSettingsControls ( preview, format, cameras );
			}
			else if ( ( gadget == PREVIEW_CAPTURE_AUDIO ) && ( preview->Inputs.Count ) )
			{
				preview->AudioCapture = ! preview->AudioCapture;

				UpdateSettingsControls ( preview, format, cameras );
			}
			else if ( ( gadget == PREVIEW_AUDIO_OUT ) && ( preview->AudioCapture ) && ( preview->Play )
					  && ( code < preview->Outputs.Count ) && ( code != preview->Outputs.Selected ) )
			{
				struct PreviewAudioRates * rates;

				preview->Outputs.Selected = code;
				rates					  = &preview->Outputs.Rates[code];

				RefreshSetGadgetAttrs ( preview->OutputRateGadget, preview->Window, NULL, CHOOSER_MaxLabels, rates->Count,
										CHOOSER_LabelArray, rates->Labels, CHOOSER_Selected, rates->Selected, TAG_END );

				UpdateSettingsControls ( preview, format, cameras );
			}
			else if ( ( gadget == PREVIEW_OUTPUT_RATE ) && ( preview->AudioCapture ) && ( preview->Play )
					  && ( code < preview->Outputs.Rates[preview->Outputs.Selected].Count ) )
			{
				preview->Outputs.Rates[preview->Outputs.Selected].Selected = code;
			}
			else if ( ( gadget == PREVIEW_AUDIO_IN ) && ( preview->AudioCapture ) && ( code < preview->Inputs.Count ) )
			{
				preview->Inputs.Selected = code;

				UpdateSettingsControls ( preview, format, cameras );
			}
			else if ( ( gadget == PREVIEW_MIC_GAIN ) && ( preview->AudioCapture )
					  && ( preview->Inputs.Gains[preview->Inputs.Selected].Step ) )
			{
				level = ReadLevelSlider ( preview->MicGainGadget, preview->Inputs.GainLevels[preview->Inputs.Selected] );

				if ( level != preview->Inputs.GainLevels[preview->Inputs.Selected] )
				{
					preview->Inputs.GainLevels[preview->Inputs.Selected]  = level;
					preview->Inputs.GainChanged[preview->Inputs.Selected] = TRUE;
				}
			}
			else if ( ( gadget == PREVIEW_RECORD_VIDEO ) && ( preview->Video ) )
			{
				preview->RecordVideo = ! preview->RecordVideo;

				UpdateSettingsControls ( preview, format, cameras );
			}
			else if ( ( gadget == PREVIEW_RECORD_AUDIO ) && ( preview->AudioCapture ) )
			{
				preview->RecordAudio = ! preview->RecordAudio;

				UpdateSettingsControls ( preview, format, cameras );
			}
			else if ( ( gadget == PREVIEW_VIDEO_FILE ) && ( preview->Video ) && ( preview->RecordVideo ) )
			{
				ChooseRecordingFile ( window, preview->VideoFileGadget, preview->VideoPath );
			}
			else if ( ( gadget == PREVIEW_AUDIO_FILE ) && ( preview->AudioCapture ) && ( preview->RecordAudio ) )
			{
				ChooseRecordingFile ( window, preview->AudioFileGadget, preview->AudioPath );
			}
			else if ( gadget == PREVIEW_START )
			{
				if ( ( preview->Video ) && ( preview->CameraLost ) && ( cameras->Count ) )
				{
					ChangeCamera ( preview, ioreq, format, cameras, cameras->Selected );
				}

				if ( ( CanStart ( preview, format ) ) && ( ConfirmRecordingFiles ( preview ) ) )
				{
					return ( SETTINGS_START );
				}
			}
		}
	}
}

// --

static S32
RestorePreviewCamera ( struct IOStdReq * ioreq, const struct VideoFormat * wanted, U32 mode,
	struct VideoFormat * actual )
{
	S32 error;

	error = VideoTest_QueryCamera ( ioreq, actual );

	if ( error )
	{
		return ( error );
	}

	if ( ( actual->VendorID != wanted->VendorID ) || ( actual->DeviceID != wanted->DeviceID ) )
	{
		return ( USB2Err_Device_Detached );
	}

	if ( ( actual->Format != wanted->Format ) || ( actual->Width != wanted->Width ) ||
		( actual->Height != wanted->Height ) || ( actual->FrameInterval != wanted->FrameInterval ) )
	{
		error = SetCameraMode ( ioreq, mode );

		if ( error )
		{
			return ( error );
		}

		error = VideoTest_QueryCamera ( ioreq, actual );

		if ( error )
		{
			return ( error );
		}
	}

	return ( CanPreviewFormat ( actual ) ? 0 : IOERR_BADLENGTH );
}

// --

static void
WaitForPreviewCamera ( struct Preview * preview, struct TimeRequest * timer, struct PreviewTimeField * fields )
{
	SetWindowTitles ( preview->Window, "USB2VideoTest - Waiting for camera", (CONST_STRPTR)-1 );

	ShowPreviewTimings ( preview->Window, fields, 0, 0, 0, 0 );

	StartPreviewTimer ( timer, 1 );
}

// --

static S32
RunPreview ( struct Preview * preview, Object * capture, struct IOStdReq * ioreq, struct TimeRequest * timer,
			 struct VideoFormat * format, struct Gadget * volumeSlider, struct PreviewTimeField * fields )
{
	struct Window *				window;
	struct TimerIFace *			clock;
	struct PreviewAudio			audio;
	struct PreviewAvi			avi;
	struct VideoTestBuffers		usbBuffers;
	struct PreviewRecording		videoFile;
	struct PreviewRecording		audioFile;
	struct PreviewTiming		timing;
	struct VideoFormat			restored;
	enum PreviewRecordingResult videoResult;

	U8 *  frame;
	U8 *  spare;
	U8 *  ready;
	STR	  title;
	U64	  start;
	U64	  now;
	U64	  captureStart;
	U64	  captureElapsed;
	U64	  captureTime;
	U64	  releaseStart;
	U64	  releaseTime;
	U64	  pictureTime;
	U64	  layoutTime;
	U64	  drawTime;
	U32	  rate;
	U32	  shown;
	U32	  directFrames;
	U32	  size;
	U32	  mask;
	ULONG windowMask;
	U32	  signals;
	U32	  event;
	U32	  gadget;
	U32	  capacity;
	U32	  failures;
	U32	  audioMask;
	U32	  selectedMode;
	S32	  pending;
	S32	  streaming;
	S32	  canReconnect;
	S32	  retrying;
	S32	  quit;
	S32	  interrupted;
	S32	  timedout;
	S32	  recordVideo;
	S32	  saveVideo;
	S32	  saveAudio;
	S32	  stopError;
	S32	  bufferError;
	S32	  direct;
	U16	  input;

	window	   = preview->Window;
	windowMask = 0;

	if ( ( ! GetAttr ( WINDOW_SigMask, capture, &windowMask ) ) || ( ! windowMask ) )
	{
		return ( RETURN_FAIL );
	}

	mask		 = windowMask | SIGBREAKF_CTRL_C;
	audio		 = (struct PreviewAudio){ 0 };
	audio.Volume = preview->Volume;
	avi			 = (struct PreviewAvi){ 0 };
	saveVideo	 = preview->Video && preview->RecordVideo;
	saveAudio	 = preview->AudioCapture && preview->RecordAudio;
	videoFile	 = (struct PreviewRecording){ 0 };
	audioFile	 = (struct PreviewRecording){ 0 };
	videoResult	 = PREVIEW_RECORD_EMPTY;
	frame		 = NULL;
	spare		 = NULL;
	capacity	 = 0;
	usbBuffers	 = (struct VideoTestBuffers){ 0 };
	canReconnect = preview->Video && LIB_IS_AT_LEAST ( (struct Library *)ioreq->io_Device,
		VIDEO_DEVICE_VERSION, VIDEO_RECONNECT_REVISION );
	retrying = FALSE;
	selectedMode = preview->Choices->ModeCount ? preview->Choices->Modes[preview->Choices->ActiveMode].Number : 0;

	if ( ( preview->Video ) && ( ! PrepareFrameBuffers ( &frame, &spare, &capacity, format->MaxFrameSize ) ) )
	{
		FreeVec ( frame );

		FreeVec ( spare );

		Printf ( "Could not allocate video frame buffers.\n" );

		return ( RETURN_FAIL );
	}

	if ( preview->Video )
	{
		bufferError = VideoTest_PrepareIsoBuffers ( ioreq, &usbBuffers );

		if ( bufferError == USB2Err_Device_Detached )
		{
			preview->CameraLost = TRUE;
		}

		if ( ( bufferError ) && ( ( ! canReconnect ) || ( bufferError != USB2Err_Device_Detached ) ) )
		{
			FreeVec ( frame );

			FreeVec ( spare );

			Printf ( "Could not prepare USB capture buffers (error %ld).\n", bufferError );

			return ( RETURN_FAIL );
		}
	}

	recordVideo = saveVideo && PreviewRecording_Stage ( &videoFile, preview->VideoPath, "video.avi" );

	if ( saveAudio )
	{
		PreviewRecording_Stage ( &audioFile, preview->AudioPath, "audio.wav" );
	}

	clock		 = NULL;
	failures	 = 0;
	pending		 = preview->Video && ! preview->CameraLost;
	streaming	 = pending;
	quit		 = FALSE;
	interrupted	 = FALSE;
	shown		 = 0;
	directFrames = 0;
	start		 = 0;
	timing		 = (struct PreviewTiming){ 0 };
	captureStart = 0;
	captureTime	 = 0;
	releaseTime	 = 0;
	pictureTime	 = 0;
	layoutTime	 = 0;
	drawTime	 = 0;

	if ( preview->Video )
	{
		clock = (PTR)GetInterface ( (PTR)timer->Request.io_Device, "main", 1, NULL );

		timing.Clock = clock;

		mask |= ( 1UL << ioreq->io_Message.mn_ReplyPort->mp_SigBit )
				| ( 1UL << timer->Request.io_Message.mn_ReplyPort->mp_SigBit );
		start = clock ? VideoTest_UptimeUS ( clock ) : 0;

		if ( streaming )
		{
			ShowPreviewRate ( preview, format, 0 );

			captureStart = StartFrame ( ioreq, timer, frame, format->MaxFrameSize, clock );
		}
		else if ( canReconnect )
		{
			WaitForPreviewCamera ( preview, timer, fields );

			retrying = TRUE;
		}
	}

	if ( ( preview->AudioCapture ) && ( ( preview->Play ) || ( audioFile.Drawer[0] ) ) )
	{
		PreviewAudio_Start ( &audio, &preview->Inputs, preview->Play, &preview->Outputs,
							 audioFile.Drawer[0] ? audioFile.Path : NULL );
	}

	if ( ( volumeSlider ) && ( ! audio.Output.Request ) )
	{
		RefreshSetGadgetAttrs ( volumeSlider, window, NULL, GA_Disabled, TRUE, TAG_END );
	}

	if ( ( ! preview->Video ) && ( ! audio.Port ) )
	{
		SetWindowTitles ( window, "USB2VideoTest - Audio unavailable", (CONST_STRPTR)-1 );
	}

	while ( ! quit )
	{
		audioMask = audio.Port ? 1UL << audio.Port->mp_SigBit : 0;

		signals = Wait ( mask | audioMask );

		interrupted = ( signals & SIGBREAKF_CTRL_C ) != 0;
		quit		= interrupted;

		while ( ( signals & windowMask ) && ( ( event = IDoMethod ( capture, WM_HANDLEINPUT, &input ) ) != WMHI_LASTMSG ) )
		{
			gadget = event & WMHI_GADGETMASK;

			if ( ( event & WMHI_CLASSMASK ) == WMHI_CLOSEWINDOW )
			{
				quit = TRUE;
			}
			else if ( ( ( event & WMHI_CLASSMASK ) == WMHI_GADGETUP ) && ( gadget == PREVIEW_VOLUME ) && ( volumeSlider ) )
			{
				preview->Volume = ReadLevelSlider ( volumeSlider, preview->Volume );
				audio.Volume	= preview->Volume;
			}
		}

		if ( ( ! quit ) && ( audio.Port ) )
		{
			PreviewAudio_Update ( &audio );

			if ( ( ! preview->Video ) && ( ! audio.Port ) )
			{
				SetWindowTitles ( window, "USB2VideoTest - Audio stopped", (CONST_STRPTR)-1 );
			}
		}

		if ( ( ! quit ) && ( retrying ) && ( CheckIO ( &timer->Request ) ) )
		{
			WaitIO ( &timer->Request );

			retrying = FALSE;

			bufferError = RestorePreviewCamera ( ioreq, format, selectedMode, &restored );

			if ( ( ! bufferError ) && ( ! PrepareFrameBuffers ( &frame, &spare, &capacity, restored.MaxFrameSize ) ) )
			{
				bufferError = USB2Err_Stack_NoMemory;
			}

			if ( ! bufferError )
			{
				bufferError = VideoTest_PrepareIsoBuffers ( ioreq, &usbBuffers );
			}

			if ( bufferError == USB2Err_Device_Detached )
			{
				StartPreviewTimer ( timer, 1 );

				retrying = TRUE;
			}
			else if ( bufferError )
			{
				SetWindowTitles ( window, "USB2VideoTest - Could not resume capture", (CONST_STRPTR)-1 );

				Printf ( "Could not resume video capture (error %ld).\n", bufferError );
			}
			else
			{
				*format = restored;
				preview->CameraLost = FALSE;
				pending = TRUE;
				streaming = TRUE;
				failures = 0;
				shown = 0;
				directFrames = 0;
				captureTime = 0;
				releaseTime = 0;
				pictureTime = 0;
				layoutTime = 0;
				drawTime = 0;

				SizePreview ( preview, format );

				ClearPreview ( preview );

				ShowPreviewRate ( preview, format, 0 );

				captureStart = StartFrame ( ioreq, timer, frame, format->MaxFrameSize, clock );

				start = captureStart;
			}
		}

		if ( ( ! quit ) && ( pending ) && ( ( CheckIO ( (PTR)ioreq ) ) || ( CheckIO ( &timer->Request ) ) ) )
		{
			timedout	   = ! CheckIO ( (PTR)ioreq );
			captureElapsed = 0;

			if ( ( clock ) && ( ! timedout ) )
			{
				captureElapsed = VideoTest_UptimeUS ( clock ) - captureStart;
			}

			FinishRequest ( (PTR)ioreq );

			FinishRequest ( &timer->Request );

			pending = FALSE;

			if ( ( timedout ) || ( ioreq->io_Error ) )
			{
				Printf ( "Video USB %04lx:%04lx %s %lu x %lu, frame interval %lu.\n", format->VendorID, format->DeviceID,
						 format->Format == VIDEO_FORMAT_MJPEG ? "MJPEG" : "YUY2", format->Width, format->Height,
						 format->FrameInterval );

				if ( ( timedout ) || ( ioreq->io_Error == VIDEO_ERR_FRAME_TIMEOUT ) )
				{
					SetWindowTitles ( window, "USB2VideoTest - Capture timed out", (CONST_STRPTR)-1 );

					Printf ( "Preview capture timed out.\n" );
				}
				else if ( ioreq->io_Error == USB2Err_Device_Detached )
				{
					preview->CameraLost = TRUE;

					SetWindowTitles ( window, "USB2VideoTest - Camera disconnected", (CONST_STRPTR)-1 );

					Printf ( "The USB camera disconnected during capture.\n" );
				}
				else
				{
					title = preview->CaptureTitle[preview->TitleSlot];

					snprintf ( title, sizeof ( preview->CaptureTitle[0] ), "USB2VideoTest - Capture failed (%ld)",
							   (long)ioreq->io_Error );

					SetWindowTitles ( window, title, (CONST_STRPTR)-1 );

					preview->TitleSlot ^= 1;

					Printf ( "Preview read failed (error %ld, %lu bytes received, buffer %lu bytes).\n", (S32)ioreq->io_Error,
							 ioreq->io_Actual, format->MaxFrameSize );
				}
			}
			else
			{
				size  = ioreq->io_Actual;
				ready = frame;
				frame = spare;
				spare = ready;

				// The JPEG datatype can keep borrowing the buffer used by the previous frame.
				releaseStart = 0;

				if ( ( preview->Trace ) && ( clock ) && ( preview->Picture ) )
				{
					releaseStart = VideoTest_UptimeUS ( timing.Clock );
				}

				ReleasePreviewPicture ( preview );

				if ( releaseStart )
				{
					releaseTime += VideoTest_UptimeUS ( timing.Clock ) - releaseStart;
				}

				// Keep the next read active while displaying the finished frame.
				captureStart = StartFrame ( ioreq, timer, frame, format->MaxFrameSize, clock );

				pending = TRUE;

				if ( recordVideo )
				{
					now = clock ? VideoTest_UptimeUS ( clock ) : 0;

					if ( ! PreviewAvi_Write ( &avi, videoFile.Path, format, ready, size, now ) )
					{
						videoResult = PreviewAvi_Close ( &avi, videoFile.Path, format );

						recordVideo = FALSE;
					}
				}

				if ( ( failures < 3 ) && ( RenderPreviewFrame ( preview, format, ready, size, &timing, &direct ) ) )
				{
					failures = 0;

					if ( clock )
					{
						shown++;
						now = VideoTest_UptimeUS ( clock );

						captureTime += captureElapsed;
						directFrames += direct;
						pictureTime += timing.PictureReady - timing.Start;
						layoutTime += timing.DrawStart - timing.PictureReady;
						drawTime += now - timing.DrawStart;

						if ( now - start >= 1000000 )
						{
							rate = 10000000ULL * shown / ( now - start );

							ShowPreviewRate ( preview, format, rate );

							ShowPreviewTimings ( window, fields, shown, captureTime, pictureTime + layoutTime, drawTime );

							if ( preview->Trace )
							{
								if ( format->Format == VIDEO_FORMAT_MJPEG )
								{
									DebugPrintF ( "USB2VideoTest: MJPEG preview %lu.%lu fps, us/frame: "
												  "datatype creation %lu, pixel prep %lu, draw call %lu, wait and dispose %lu"
												  " (%lu/%lu direct)\n",
												  rate / 10, rate % 10, (U32)( pictureTime / shown ),
												  (U32)( layoutTime / shown ), (U32)( drawTime / shown ),
												  (U32)( releaseTime / shown ), directFrames, shown );
								}
								else
								{
									DebugPrintF ( "USB2VideoTest: YUY2 preview us/frame: conversion %lu, draw call %lu\n",
												  (U32)( pictureTime / shown ), (U32)( drawTime / shown ) );
								}
							}

							shown		 = 0;
							directFrames = 0;
							start		 = now;
							captureTime	 = 0;
							releaseTime	 = 0;
							pictureTime	 = 0;
							layoutTime	 = 0;
							drawTime	 = 0;
						}
					}
				}
				else if ( ( failures < 3 ) && ( ++failures == 3 ) )
				{
					SetWindowTitles ( window,
									  recordVideo ? "USB2VideoTest - Preview unavailable, recording AVI"
												  : "USB2VideoTest - Preview unavailable",
									  (CONST_STRPTR)-1 );

					if ( format->Format == VIDEO_FORMAT_MJPEG )
					{
						Printf ( "Could not display the JPEG frames. Check DEVS:DataTypes for the JPEG datatype.\n" );
					}
					else
					{
						Printf ( "Could not display the YUY2 frames.\n" );
					}
				}

				if ( ( failures == 3 ) && ( ! recordVideo ) )
				{
					FinishRequest ( (PTR)ioreq );

					FinishRequest ( &timer->Request );

					pending = FALSE;

					SetWindowTitles ( window, "USB2VideoTest - Video stopped", (CONST_STRPTR)-1 );
				}
			}
		}

		if ( ( pending ) && ( quit ) )
		{
			FinishRequest ( (PTR)ioreq );

			FinishRequest ( &timer->Request );

			pending = FALSE;

			if ( ( recordVideo ) && ( ! ioreq->io_Error ) )
			{
				now = clock ? VideoTest_UptimeUS ( clock ) : 0;

				PreviewAvi_Write ( &avi, videoFile.Path, format, frame, ioreq->io_Actual, now );
			}
		}
		if ( ( streaming ) && ( ! pending ) )
		{
			if ( preview->Trace )
			{
				DebugPrintF ( "USB2VideoTest: stopping video stream\n" );
			}

			stopError = VideoTest_StopCapture ( ioreq, &usbBuffers );

			if ( preview->Trace )
			{
				VideoTest_ReportCaptureStats ( ioreq );
			}

			if ( stopError == USB2Err_Device_Detached )
			{
				preview->CameraLost = TRUE;
			}
			else if ( ( stopError ) && ( stopError != IOERR_NOCMD ) )
			{
				Printf ( "Could not stop the video stream (error %ld).\n", stopError );
			}

			if ( recordVideo )
			{
				videoResult = PreviewAvi_Close ( &avi, videoFile.Path, format );

				recordVideo = FALSE;
			}

			streaming = FALSE;

			if ( ( ! quit ) && ( canReconnect ) && ( preview->CameraLost ) )
			{
				ReleasePreviewPicture ( preview );

				WaitForPreviewCamera ( preview, timer, fields );

				retrying = TRUE;
			}
		}
	}

	if ( retrying )
	{
		FinishRequest ( &timer->Request );
	}

	if ( preview->Trace )
	{
		DebugPrintF ( "USB2VideoTest: closing preview audio\n" );
	}

	PreviewAudio_Stop ( &audio );

	if ( saveVideo )
	{
		PreviewRecording_Finish ( &videoFile, window, "AVI", videoResult );
	}

	if ( saveAudio )
	{
		PreviewRecording_Finish ( &audioFile, window, "WAV", audio.RecordingResult );
	}

	ReleasePreviewPicture ( preview );

	FreeVec ( frame );

	FreeVec ( spare );

	if ( clock )
	{
		DropInterface ( (PTR)clock );
	}

	return ( interrupted ? RETURN_WARN : RETURN_OK );
}

// --

static PTR
AddSettingsChild ( Object * parent, Object * child, U32 weight )
{
	struct lmAddChild add;
	struct TagItem	  attrs[] = { { CHILD_WeightedWidth, weight }, { TAG_END, 0 } };

	if ( ! child )
	{
		return ( NULL );
	}

	add.MethodID	   = LM_ADDCHILD;
	add.lm_Window	   = NULL;
	add.lm_Object	   = child;
	add.lm_ObjectAttrs = attrs;

	if ( ! IDoMethodA ( parent, (Msg)&add ) )
	{
		DisposeObject ( child );

		return ( NULL );
	}

	return ( child );
}

// --

static Object *
AddSettingsGroup ( Object * parent, Class * layout, const char * label, S32 even )
{
	Object * group;

	if ( label )
	{
		group = NewObject ( layout, NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT, LAYOUT_BevelStyle, BVS_GROUP, LAYOUT_Label,
							label, LAYOUT_SpaceInner, TRUE, LAYOUT_SpaceOuter, TRUE, TAG_END );
	}
	else
	{
		group = NewObject ( layout, NULL, LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ, LAYOUT_SpaceInner, TRUE, LAYOUT_EvenSize,
							even, TAG_END );
	}

	return ( AddSettingsChild ( parent, group, 100 ) );
}

// --

static struct Gadget *
AddSettingsCheck ( Object * row, Class * checkbox, U32 id, const char * label )
{
	Object * control;

	control = NewObject ( checkbox, NULL, GA_ID, id, GA_Text, label, GA_RelVerify, TRUE, CHECKBOX_TextPlace, PLACETEXT_RIGHT,
						  TAG_END );

	return ( AddSettingsChild ( row, control, 0 ) );
}

// --

static struct Gadget *
AddSettingsChooser ( Object * row, Class * chooser, U32 id, STR * labels, U32 selected )
{
	Object * control;

	U32 count;

	for ( count = 0; labels[count]; count++ )
	{
	}

	control = NewObject ( chooser, NULL, GA_ID, id, GA_RelVerify, TRUE, CHOOSER_PopUp, TRUE, CHOOSER_LabelArray, labels,
						  CHOOSER_MaxLabels, MAX ( 1, count ), CHOOSER_Selected, selected, TAG_END );

	return ( AddSettingsChild ( row, control, 100 ) );
}

// --

static struct Gadget *
AddSettingsFile ( Object * row, Class * getfile, U32 id, const char * path, const char * title )
{
	Object * control;

	control = NewObject ( getfile, NULL, GA_ID, id, GA_RelVerify, TRUE, GETFILE_ReadOnly, TRUE, GETFILE_DoSaveMode, TRUE,
						  GETFILE_TitleText, title, GETFILE_FullFile, path, TAG_END );

	return ( AddSettingsChild ( row, control, 100 ) );
}

// --

static S32
BuildSettingsControls ( struct Preview * preview, Object * layout, Class ** classes, STR * cameraLabels, U32 camera )
{
	struct PreviewAudioRates * rates;
	Object *				   group;
	Object *				   row;
	Object *				   button;

	group = AddSettingsGroup ( layout, classes[SETTINGS_LAYOUT], "Video Capture", FALSE );

	if ( ! group )
	{
		return ( FALSE );
	}

	row = AddSettingsGroup ( group, classes[SETTINGS_LAYOUT], NULL, FALSE );

	if ( ! row )
	{
		return ( FALSE );
	}

	preview->VideoGadget = AddSettingsCheck ( row, classes[SETTINGS_CHECKBOX], PREVIEW_VIDEO, "Enable" );

	preview->CameraGadget = AddSettingsChooser ( row, classes[SETTINGS_CHOOSER], PREVIEW_CAMERA, cameraLabels, camera );

	if ( ( ! preview->VideoGadget ) || ( ! preview->CameraGadget ) )
	{
		return ( FALSE );
	}

	if ( preview->VideoAvailable )
	{
		row = AddSettingsGroup ( group, classes[SETTINGS_LAYOUT], NULL, TRUE );

		if ( ! row )
		{
			return ( FALSE );
		}

		preview->FormatGadget
			= AddSettingsChooser ( row, classes[SETTINGS_CHOOSER], PREVIEW_FORMAT, preview->Choices->FormatLabels, 0 );

		preview->SizeGadget = AddSettingsChooser ( row, classes[SETTINGS_CHOOSER], PREVIEW_SIZE, CurrentModeLabels, 0 );

		preview->SpeedGadget = AddSettingsChooser ( row, classes[SETTINGS_CHOOSER], PREVIEW_SPEED, CurrentModeLabels, 0 );

		if ( ( ! preview->FormatGadget ) || ( ! preview->SizeGadget ) || ( ! preview->SpeedGadget ) )
		{
			return ( FALSE );
		}

		row = AddSettingsGroup ( group, classes[SETTINGS_LAYOUT], NULL, FALSE );

		if ( ! row )
		{
			return ( FALSE );
		}

		preview->RecordVideoGadget = AddSettingsCheck ( row, classes[SETTINGS_CHECKBOX], PREVIEW_RECORD_VIDEO, "Save AVI" );

		preview->VideoFileGadget
			= AddSettingsFile ( row, classes[SETTINGS_GETFILE], PREVIEW_VIDEO_FILE, preview->VideoPath, "Save AVI recording" );

		if ( ( ! preview->RecordVideoGadget ) || ( ! preview->VideoFileGadget ) )
		{
			return ( FALSE );
		}
	}

	group = AddSettingsGroup ( layout, classes[SETTINGS_LAYOUT], "Audio Capture", FALSE );

	if ( ! group )
	{
		return ( FALSE );
	}

	row = AddSettingsGroup ( group, classes[SETTINGS_LAYOUT], NULL, FALSE );

	if ( ! row )
	{
		return ( FALSE );
	}

	preview->AudioCaptureGadget = AddSettingsCheck ( row, classes[SETTINGS_CHECKBOX], PREVIEW_CAPTURE_AUDIO, "Enable" );

	preview->InputGadget = AddSettingsChooser ( row, classes[SETTINGS_CHOOSER], PREVIEW_AUDIO_IN, preview->Inputs.Labels,
												preview->Inputs.Selected );

	if ( ( ! preview->AudioCaptureGadget ) || ( ! preview->InputGadget ) )
	{
		return ( FALSE );
	}

	preview->MicGainGadget = AddSettingsChild ( group,
												NewObject ( classes[SETTINGS_SLIDER], NULL, GA_ID, PREVIEW_MIC_GAIN,
															GA_RelVerify, TRUE, SLIDER_Orientation, SLIDER_HORIZONTAL,
															SLIDER_Min, 0, SLIDER_Max, 100, SLIDER_LevelFormat, "Mic Gain %ld%%",
															SLIDER_LevelMaxLen, 14, SLIDER_LevelPlace, PLACETEXT_IN, TAG_END ),
												100 );

	if ( ! preview->MicGainGadget )
	{
		return ( FALSE );
	}

	row = AddSettingsGroup ( group, classes[SETTINGS_LAYOUT], NULL, FALSE );

	if ( ! row )
	{
		return ( FALSE );
	}

	preview->RecordAudioGadget = AddSettingsCheck ( row, classes[SETTINGS_CHECKBOX], PREVIEW_RECORD_AUDIO, "Save WAV" );

	preview->AudioFileGadget
		= AddSettingsFile ( row, classes[SETTINGS_GETFILE], PREVIEW_AUDIO_FILE, preview->AudioPath, "Save WAV recording" );

	if ( ( ! preview->RecordAudioGadget ) || ( ! preview->AudioFileGadget ) )
	{
		return ( FALSE );
	}

	group = AddSettingsGroup ( layout, classes[SETTINGS_LAYOUT], "Audio Output", FALSE );

	if ( ! group )
	{
		return ( FALSE );
	}

	row = AddSettingsGroup ( group, classes[SETTINGS_LAYOUT], NULL, FALSE );

	if ( ! row )
	{
		return ( FALSE );
	}

	preview->PlayGadget = AddSettingsCheck ( row, classes[SETTINGS_CHECKBOX], PREVIEW_PLAY, "Enable" );

	preview->OutputGadget = AddSettingsChooser ( row, classes[SETTINGS_CHOOSER], PREVIEW_AUDIO_OUT, preview->Outputs.Labels,
												 preview->Outputs.Selected );

	rates = &preview->Outputs.Rates[preview->Outputs.Selected];

	preview->OutputRateGadget
		= AddSettingsChooser ( row, classes[SETTINGS_CHOOSER], PREVIEW_OUTPUT_RATE, rates->Labels, rates->Selected );

	if ( ( ! preview->PlayGadget ) || ( ! preview->OutputGadget ) || ( ! preview->OutputRateGadget ) )
	{
		return ( FALSE );
	}

	button = NewObject ( classes[SETTINGS_BUTTON], NULL, GA_ID, PREVIEW_START, GA_Text, "Start", GA_RelVerify, TRUE, TAG_END );

	preview->StartGadget = AddSettingsChild ( layout, button, 0 );

	return ( preview->StartGadget != NULL );
}

// --

static S32
AddPreviewTimingRow ( Object * layout, Class ** classes, struct PreviewTimeField * fields )
{
	Object * row;
	Object * cell;

	U32 index;

	row = AddSettingsChild ( layout,
							 NewObject ( classes[SETTINGS_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
										 LAYOUT_BevelStyle, BVS_THIN, LAYOUT_EvenSize, TRUE, TAG_END ),
							 100 );

	if ( ! row )
	{
		return ( FALSE );
	}

	for ( index = 0; index < PREVIEW_TIME_FIELDS; index++ )
	{
		cell = AddSettingsChild ( row, NewObject ( classes[SETTINGS_LAYOUT], NULL, LAYOUT_BevelStyle, BVS_THIN, TAG_END ), 100 );

		if ( ! cell )
		{
			return ( FALSE );
		}

		snprintf ( fields[index].Text[0], sizeof ( fields[index].Text[0] ), "%s: -- ms", PreviewTimeLabels[index] );

		fields[index].Gadget
			= AddSettingsChild ( cell,
								 NewObject ( classes[SETTINGS_BUTTON], NULL, GA_Text, fields[index].Text[0], GA_ReadOnly, TRUE,
											 BUTTON_Transparent, TRUE, BUTTON_BevelStyle, BVS_NONE, TAG_END ),
								 100 );

		if ( ! fields[index].Gadget )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}

// --

static S32
ShowCaptureWindow ( struct Preview * preview, struct Screen * screen, struct IOStdReq * ioreq, struct TimeRequest * timer,
					struct VideoFormat * format, Class ** classes )
{
	struct Gadget *			volumeSlider;
	struct IBox *			area;
	Object *				surface;
	Object *				layout;
	Object *				capture;
	struct PreviewTimeField fields[PREVIEW_TIME_FIELDS];

	CONST_STRPTR title;
	ULONG		 bounds;
	S32			 width;
	S32			 height;
	S32			 showVolume;
	S32			 result;

	showVolume	 = preview->Video || ( preview->AudioCapture && preview->Play );
	result		 = RETURN_FAIL;
	capture		 = NULL;
	surface		 = NULL;
	volumeSlider = NULL;
	width		 = screen->Width - screen->WBorLeft - screen->WBorRight - 16;
	height		 = screen->Height - screen->BarHeight - screen->WBorTop - screen->WBorBottom - screen->Font->ta_YSize - 16;

	if ( showVolume )
	{
		height -= screen->Font->ta_YSize + 12;
	}

	if ( preview->Video )
	{
		height -= screen->Font->ta_YSize + 12;
	}

	if ( ( width < 1 ) || ( height < 1 ) )
	{
		Printf ( "Cannot fit the preview on the public screen.\n" );

		return ( RETURN_FAIL );
	}

	if ( preview->Video )
	{
		width  = MIN ( width, PREVIEW_WIDTH );
		height = MIN ( height, PREVIEW_HEIGHT );
		title  = "USB2VideoTest - Video";
	}
	else
	{
		width  = MIN ( width, 384 );
		height = MIN ( height, screen->Font->ta_YSize + 4 );
		title  = preview->RecordAudio ? "USB2VideoTest - Recording audio" : "USB2VideoTest - Monitoring audio";
	}

	layout = NewObject ( classes[SETTINGS_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT, LAYOUT_SpaceInner, TRUE,
						 LAYOUT_SpaceOuter, TRUE, TAG_END );

	if ( ! layout )
	{
		goto bailout;
	}

	if ( ( preview->Video ) || ( ! showVolume ) )
	{
		surface = AddSettingsChild ( layout,
									 NewObject ( classes[SETTINGS_SPACE], NULL, SPACE_MinWidth, width, SPACE_MinHeight, height,
												 SPACE_Transparent, TRUE, SPACE_BevelStyle,
												 preview->Video ? BVS_FIELD : BVS_NONE, SPACE_DomainBevel, TRUE, TAG_END ),
									 100 );

		if ( ! surface )
		{
			goto bailout;
		}
	}

	if ( showVolume )
	{
		volumeSlider = AddSettingsChild ( layout,
										  NewObject ( classes[SETTINGS_SLIDER], NULL, GA_ID, PREVIEW_VOLUME, GA_RelVerify, TRUE,
													  SLIDER_Orientation, SLIDER_HORIZONTAL, SLIDER_Min, 0, SLIDER_Max, 100,
													  SLIDER_Level, preview->Volume, SLIDER_LevelFormat, "Volume %ld%%",
													  SLIDER_LevelMaxLen, 12, SLIDER_LevelPlace, PLACETEXT_IN, TAG_END ),
										  100 );

		if ( ! volumeSlider )
		{
			goto bailout;
		}
	}

	memset ( fields, 0, sizeof ( fields ) );

	if ( ( preview->Video ) && ( ! AddPreviewTimingRow ( layout, classes, fields ) ) )
	{
		goto bailout;
	}

	capture = NewObject ( classes[SETTINGS_WINDOW], NULL, WA_PubScreen, screen, WA_Title, title, WA_AutoAdjust, TRUE, WA_DragBar,
						  TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_Activate, TRUE, WA_SmartRefresh, TRUE, WA_IDCMP,
						  IDCMP_CLOSEWINDOW | IDCMP_GADGETUP, TAG_END );

	if ( ! capture )
	{
		goto bailout;
	}

	SetAttrs ( capture, WINDOW_Layout, layout, TAG_END );

	layout = NULL;

	preview->Window = (PTR)IDoMethod ( capture, WM_OPEN, NULL );

	if ( ! preview->Window )
	{
		goto bailout;
	}

	preview->Picture   = NULL;
	preview->RGB	   = NULL;
	preview->Bitmap	   = NULL;
	preview->TitleSlot = 0;

	if ( preview->Video )
	{
		bounds = 0;

		if ( ( ! GetAttr ( SPACE_AreaBox, surface, &bounds ) ) || ( ! bounds ) )
		{
			goto bailout;
		}

		area = (PTR)bounds;

		if ( ( area->Width < 1 ) || ( area->Height < 1 ) )
		{
			goto bailout;
		}

		preview->MaxWidth  = MIN ( width, area->Width );
		preview->MaxHeight = MIN ( height, area->Height );
		preview->Left	   = area->Left + ( area->Width - preview->MaxWidth ) / 2;
		preview->Top	   = area->Top + ( area->Height - preview->MaxHeight ) / 2;

		SizePreview ( preview, format );

		ClearPreview ( preview );
	}

	result = RunPreview ( preview, capture, ioreq, timer, format, volumeSlider, fields );

bailout:
	if ( capture )
	{
		if ( preview->Trace )
		{
			DebugPrintF ( "USB2VideoTest: releasing capture window and frame buffers\n" );
		}

		ReleasePreviewPicture ( preview );

		if ( preview->RGB )
		{
			WaitBlit ();

			FreeVec ( preview->RGB );

			preview->RGB = NULL;
		}

		DisposeObject ( capture );
	}

	if ( layout )
	{
		DisposeObject ( layout );
	}

	if ( result == RETURN_FAIL )
	{
		Printf ( "Could not run the capture window.\n" );
	}

	return ( result );
}

// --

static enum SettingsAction
ShowSettingsWindow ( struct Preview * preview, struct Screen * screen, struct IOStdReq ** ioreq, struct TimeRequest * timer,
					 struct VideoFormat * format, struct PreviewCameras * cameras )
{
	static const char * const names[SETTINGS_CLASS_COUNT] = { [SETTINGS_WINDOW]	  = "window.class",
															  [SETTINGS_LAYOUT]	  = "gadgets/layout.gadget",
															  [SETTINGS_CHECKBOX] = "gadgets/checkbox.gadget",
															  [SETTINGS_CHOOSER]  = "gadgets/chooser.gadget",
															  [SETTINGS_GETFILE]  = "gadgets/getfile.gadget",
															  [SETTINGS_BUTTON]	  = "gadgets/button.gadget",
															  [SETTINGS_SLIDER]	  = "gadgets/slider.gadget",
															  [SETTINGS_SPACE]	  = "gadgets/space.gadget" };

	struct ClassLibrary * bases[SETTINGS_CLASS_COUNT];
	struct Window *		  settings;
	Class *				  classes[SETTINGS_CLASS_COUNT];
	Object *			  window;
	Object *			  layout;
	enum SettingsAction	  action;

	STR	  cameraLabel[2];
	STR * cameraLabels;
	U32	  index;
	S32	  result;
	U16	  input;

	action					= SETTINGS_ERROR;
	preview->SizeMenuLabels = CurrentModeLabels;
	cameraLabel[0]			= preview->VideoAvailable ? "Selected camera" : "No video input";
	cameraLabel[1]			= NULL;
	cameraLabels			= cameras->Count ? cameras->Labels : cameraLabel;
	window					= NULL;
	layout					= NULL;

	for ( index = 0; index < SETTINGS_CLASS_COUNT; index++ )
	{
		bases[index] = OpenClass ( names[index], 50, &classes[index] );

		if ( ! bases[index] )
		{
			Printf ( "Could not open %s for the settings window.\n", names[index] );

			goto bailout;
		}
	}

	window = NewObject ( classes[SETTINGS_WINDOW], NULL, WA_PubScreen, screen, WA_Title, "USB2VideoTest - Settings",
						 WA_InnerWidth, MIN ( 520, MAX ( 1, screen->Width - screen->WBorLeft - screen->WBorRight - 4 ) ),
						 WA_AutoAdjust, TRUE, WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_SizeGadget, TRUE,
						 WA_Activate, TRUE, WINDOW_LockHeight, TRUE, WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP, TAG_END );

	if ( ! window )
	{
		goto bailout;
	}

	layout = NewObject ( classes[SETTINGS_LAYOUT], NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT, LAYOUT_HorizAlignment,
						 LAYOUT_ALIGN_RIGHT, LAYOUT_SpaceInner, TRUE, LAYOUT_SpaceOuter, TRUE, TAG_END );

	if ( ( ! layout ) || ( ! BuildSettingsControls ( preview, layout, classes, cameraLabels, cameras->Selected ) ) )
	{
		goto bailout;
	}

	SetAttrs ( window, WINDOW_Layout, layout, TAG_END );
	layout = NULL;

	settings = (PTR)IDoMethod ( window, WM_OPEN, NULL );

	if ( ! settings )
	{
		goto bailout;
	}

	preview->Window = settings;

	if ( preview->SizeGadget )
	{
		RefreshModeChoices ( preview );
	}

	UpdateSettingsControls ( preview, format, cameras );

	for ( ;; )
	{
		action = RunSettings ( preview, window, ioreq, format, cameras );

		if ( action != SETTINGS_START )
		{
			break;
		}

		if ( ! HideWindow ( settings ) )
		{
			SetWindowTitles ( settings, "USB2VideoTest - Could not hide settings", (CONST_STRPTR)-1 );

			continue;
		}

		result = ShowCaptureWindow ( preview, screen, *ioreq, timer, format, classes );

		preview->Window = settings;

		if ( result == RETURN_WARN )
		{
			action = SETTINGS_CLOSE;
			break;
		}

		// Discard clicks queued before the settings window was hidden.
		while ( IDoMethod ( window, WM_HANDLEINPUT, &input ) != WMHI_LASTMSG )
		{
		}

		if ( ! ShowWindow ( settings, WINDOW_FRONTMOST ) )
		{
			action = SETTINGS_ERROR;
			break;
		}

		if ( preview->CameraLost )
		{
			SetWindowTitles ( settings, "USB2VideoTest - Camera unavailable", (CONST_STRPTR)-1 );
		}
		else
		{
			SetWindowTitles ( settings,
							  ( result == RETURN_OK ) ? "USB2VideoTest - Settings" : "USB2VideoTest - Could not start capture",
							  (CONST_STRPTR)-1 );
		}

		UpdateSettingsControls ( preview, format, cameras );

		ActivateWindow ( settings );
	}

bailout:
	if ( action == SETTINGS_ERROR )
	{
		Printf ( "Could not open or operate the settings window.\n" );
	}

	if ( layout )
	{
		DisposeObject ( layout );
	}

	if ( window )
	{
		if ( preview->Trace )
		{
			DebugPrintF ( "USB2VideoTest: disposing settings window\n" );
		}

		DisposeObject ( window );
	}

	while ( index )
	{
		index--;

		if ( preview->Trace )
		{
			DebugPrintF ( "USB2VideoTest: closing %s\n", names[index] );
		}

		CloseClass ( bases[index] );
	}

	preview->Window = NULL;

	return ( action );
}

// --

S32
VideoTest_Preview ( struct IOStdReq ** ioreq, struct TimeRequest * timer, const struct VideoFormat * format,
					struct PreviewCameras * cameras, S32 trace )
{
	struct Screen *		screen;
	struct VideoFormat	selected;
	struct Preview *	preview;
	enum SettingsAction action;

	S32 result;

	result	 = RETURN_FAIL;
	selected = *format;
	preview	 = AllocVecTags ( sizeof ( *preview ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! preview )
	{
		Printf ( "Could not allocate preview settings.\n" );

		return ( RETURN_FAIL );
	}

	preview->Volume = 70;
	preview->Trace	= trace;

	strcpy ( preview->VideoPath, "RAM:usb2video.avi" );

	strcpy ( preview->AudioPath, "RAM:usb2video-audio.wav" );

	preview->VideoAvailable = CanPreviewFormat ( format );

	preview->Video = preview->VideoAvailable;

	IIntuition = OpenPreviewLibrary ( "intuition.library" );
	IGraphics  = NULL;
	IDataTypes = NULL;

	if ( preview->VideoAvailable )
	{
		IGraphics  = OpenPreviewLibrary ( "graphics.library" );
		IDataTypes = OpenPreviewLibrary ( "datatypes.library" );
	}

	if ( ( ! IIntuition ) || ( ( preview->VideoAvailable ) && ( ( ! IGraphics ) || ( ! IDataTypes ) ) ) )
	{
		Printf ( "Could not open the preview libraries.\n" );
		goto close_libraries;
	}

	preview->Choices = calloc ( 1, sizeof ( *preview->Choices ) );

	if ( ( ! preview->Choices )
		 || ( ( preview->VideoAvailable ) && ( ! LoadPreviewModes ( preview->Choices, *ioreq, format ) ) ) )
	{
		Printf ( "Could not load camera modes for the preview.\n" );
		goto close_libraries;
	}

	PreviewAudio_Find ( &preview->Inputs, &preview->Outputs, format->VendorID, format->DeviceID );

	screen = LockPubScreen ( NULL );

	if ( ! screen )
	{
		Printf ( "Could not lock the public screen.\n" );
		goto close_libraries;
	}

	action = ShowSettingsWindow ( preview, screen, ioreq, timer, &selected, cameras );

	result = ( action == SETTINGS_ERROR ) ? RETURN_FAIL : RETURN_OK;

	UnlockPubScreen ( NULL, screen );

close_libraries:
	if ( trace )
	{
		DebugPrintF ( "USB2VideoTest: closing preview library interfaces\n" );
	}

	ClosePreviewLibrary ( (PTR)IDataTypes );

	ClosePreviewLibrary ( (PTR)IGraphics );

	ClosePreviewLibrary ( (PTR)IIntuition );

	FreePreviewModes ( preview->Choices );

	FreeVec ( preview );

	return ( result );
}

// --
