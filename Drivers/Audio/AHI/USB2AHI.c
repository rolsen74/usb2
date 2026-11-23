
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <proto/exec.h>
#include <interfaces/dos.h>
#include <interfaces/utility.h>
#include <interfaces/ahi_sub.h>
#include <dos/dostags.h>
#include <exec/exectags.h>
#include <exec/resident.h>
#include <devices/usb2.h>
#include "../inc/AudioDevice.h"

// --

#ifdef BLUETOOTH_AHI
#include <interfaces/bluetooth2.h>

#define DRIVER_NAME "bluetooth2.audio"
#define DRIVER_ID DRIVER_NAME " 6.1 (23.11.2026)\r\n"
#define WORKER_NAME "Bluetooth2 AHI playback"
#define OUTPUT_NAME "Bluetooth A2DP"
#define DEFAULT_RATE 48000
#else
#define DRIVER_NAME "usb2.audio"
#define DRIVER_ID DRIVER_NAME " 6.1 (23.11.2026)\r\n"
#define WORKER_NAME "USB2 AHI playback"
#define OUTPUT_NAME "USB audio"
#define DEFAULT_RATE 48000
#endif
#define UNUSED __attribute__((unused))
#define USED __attribute__((used))

struct ExecIFace *IExec;

struct Playback
{
	struct IOStdReq *Request;
	struct MsgPort *CallerPort;
	struct SignalSemaphore Lifetime;
	struct Task *Worker;
	struct Task *Parent;
	struct Library *DOSBase;
	struct Library *UtilityBase;
	struct DOSIFace *DOS;
	struct UtilityIFace *Utility;
	struct AHIAudioCtrlDrv *Audio;
	struct AudioFormat Format;
#ifdef BLUETOOTH_AHI
	struct Bluetooth2PCMIFace *Bluetooth;
	struct BTAudioPCM Stream;
	ULONG Unit;
	ULONG PCMOffset;
	ULONG PCMLength;
#else
	ULONG WriteDepth;
#endif
	U8 *Mix;
	U8 *PCM;
	ULONG StartMask;
	BOOL Started;
};

// --

static ULONG
MixPlayback( struct Playback *play, U8 *pcm )
{
	struct AHIAudioCtrlDrv *audio;

	ULONG frames;
	ULONG index;
	ULONG step;
	int32 left;
	int32 right;
	WORD *mix;
	BOOL stereo;

	audio = play->Audio;

	Forbid();

	if ( audio->ahiac_PlayerFunc )
	{
		play->Utility->CallHookPkt( audio->ahiac_PlayerFunc, audio, NULL );
	}

	frames = audio->ahiac_BuffSamples;
	stereo = audio->ahiac_BuffType == AHIST_S16S;
	step = stereo ? 2 : 1;

	if ( ( ! frames ) || ( ( audio->ahiac_BuffType != AHIST_M16S ) && ( ! stereo ) ) ||
		( frames > audio->ahiac_MaxBuffSamples ) || ( frames > audio->ahiac_BuffSize / ( step * 2 ) ) )
	{
		Permit();

		return ( 0 );
	}

	play->Utility->CallHookPkt( audio->ahiac_MixerFunc, audio, play->Mix );

	Permit();

	mix = (APTR) play->Mix;

	for( index = 0; index < frames; index++ )
	{
		left = mix[0];
		right = stereo ? mix[1] : left;
		mix += step;

		if ( play->Format.Channels == 1 )
		{
			left = ( left + right ) / 2;
		}

		*pcm++ = (U8) left;
		*pcm++ = (U16) left >> 8;

		if ( play->Format.Channels == 2 )
		{
			*pcm++ = (U8) right;
			*pcm++ = (U16) right >> 8;
		}
	}

	return ( frames * play->Format.Channels * 2 );
}

// --

#ifdef BLUETOOTH_AHI
#include "BluetoothPlayback.c"
#else
static void
Playback_Run( struct Playback *play, struct IOStdReq *next )
{
	struct IOStdReq *requests[2];
	struct IOStdReq *io;

	ULONG depth;
	ULONG count;
	ULONG head;
	ULONG slot;
	ULONG length;
	ULONG mask;
	ULONG bufferBytes;
	BOOL sourceEnded;

	requests[0] = play->Request;
	requests[1] = next;
	depth = next ? 2 : 1;
	count = 0;
	head = 0;
	sourceEnded = FALSE;
	mask = 1UL << play->Request->io_Message.mn_ReplyPort->mp_SigBit;
	bufferBytes = play->Audio->ahiac_MaxBuffSamples * play->Format.Channels * 2;

	while( ! ( SetSignal( 0, 0 ) & SIGBREAKF_CTRL_C ) )
	{
		while( ( count < depth ) && ( ! sourceEnded ) )
		{
			slot = ( head + count ) % depth;
			io = requests[slot];
			io->io_Data = play->PCM + slot * bufferBytes;

			length = MixPlayback( play, io->io_Data );

			if ( ! length )
			{
				sourceEnded = TRUE;

				break;
			}

			io->io_Command = ( depth == 2 ) ? AUDIO_CMD_QUEUE_WRITE : CMD_WRITE;
			io->io_Length = length;
			count++;

			SendIO( (APTR) io );
		}

		if ( ! count )
		{
			break;
		}

		io = requests[head];

		if ( ! CheckIO( (APTR) io ) )
		{
			if ( Wait( SIGBREAKF_CTRL_C | mask ) & SIGBREAKF_CTRL_C )
			{
				break;
			}

			continue;
		}

		WaitIO( (APTR) io );

		head = ( head + 1 ) % depth;
		count--;

		if ( io->io_Error == AUDIO_ERR_SHORT_WRITE )
		{
			continue;
		}

		if ( ( io->io_Error ) || ( io->io_Actual != io->io_Length ) )
		{
			DebugPrintF( "USB2 AHI: playback stopped, error %ld, wrote %lu of %lu bytes.\n",
				(LONG) io->io_Error, io->io_Actual, io->io_Length );

			break;
		}
	}

	for( slot = 0; slot < count; slot++ )
	{
		io = requests[( head + slot ) % depth];

		if ( ! CheckIO( (APTR) io ) )
		{
			AbortIO( (APTR) io );
		}
	}

	while( count )
	{
		WaitIO( (APTR) requests[head] );

		head = ( head + 1 ) % depth;
		count--;
	}
}
#endif

// --

static int32
Playback_Entry( void )
{
	struct Task *task;
	struct Playback *play;
#ifndef BLUETOOTH_AHI
	struct MsgPort *port;
	struct IOStdReq *next;
#endif

	task = FindTask( NULL );
	play = task->tc_UserData;

	ObtainSemaphore( & play->Lifetime );

#ifdef BLUETOOTH_AHI
	play->Worker = task;

	Signal( play->Parent, play->StartMask );

	if ( ! ( Wait( SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D ) & SIGBREAKF_CTRL_C ) )
	{
		Playback_Run( play );

		if ( ! play->Started )
		{
			Signal( play->Parent, play->StartMask );
		}
	}
#else
	port = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( port )
	{
		play->Request->io_Message.mn_ReplyPort = port;
		play->Worker = task;
		play->Started = TRUE;
	}

	Signal( play->Parent, play->StartMask );

	if ( port )
	{
		next = NULL;

		if ( play->WriteDepth == 2 )
		{
			next = AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( *next ),
				ASOIOR_ReplyPort, port, TAG_END );

			if ( next )
			{
				next->io_Device = play->Request->io_Device;
				next->io_Unit = play->Request->io_Unit;
			}
		}

		Playback_Run( play, next );

		if ( next )
		{
			FreeSysObject( ASOT_IOREQUEST, next );
		}

		play->Request->io_Message.mn_ReplyPort = play->CallerPort;

		FreeSysObject( ASOT_PORT, port );
	}

#endif

	Forbid();

	play->Worker = NULL;

	ReleaseSemaphore( & play->Lifetime );

	// DOS will dismantle the process and release Forbid on return.
	return ( 0 );
}

// --

static ULONG
AHI_Stop( struct AHIsubIFace *self UNUSED, ULONG flags, struct AHIAudioCtrlDrv *audio )
{
	struct Playback *play;

	play = audio->ahiac_DriverData;

	if ( ( ! play ) || ( ! ( flags & AHISF_PLAY ) ) )
	{
		return ( AHIE_OK );
	}

	Forbid();

	if ( play->Worker )
	{
		Signal( play->Worker, SIGBREAKF_CTRL_C );
	}

	Permit();

	ObtainSemaphore( & play->Lifetime );

	ReleaseSemaphore( & play->Lifetime );

	FreeVec( play->PCM );

	FreeVec( play->Mix );

	play->PCM = NULL;
	play->Mix = NULL;

	return ( AHIE_OK );
}

// --

static ULONG
AHI_Start( struct AHIsubIFace *self, ULONG flags, struct AHIAudioCtrlDrv *audio )
{
	struct Playback *play;
	struct Process *process;

	LONG signal;
	ULONG pcmBytes;
	BOOL ready;

	play = audio->ahiac_DriverData;

	if ( ( ! play ) || ( flags & AHISF_RECORD ) )
	{
		return ( AHIE_HALFDUPLEX );
	}

	if ( ! ( flags & AHISF_PLAY ) )
	{
		return ( AHIE_OK );
	}

	AHI_Stop( self, AHISF_PLAY, audio );

	if ( ( ! audio->ahiac_MixerFunc ) || ( ! audio->ahiac_BuffSize ) ||
		( ! audio->ahiac_MaxBuffSamples ) || ( audio->ahiac_MaxBuffSamples > 65535 ) )
	{
		return ( AHIE_BADSAMPLETYPE );
	}

	signal = AllocSignal( -1 );

	if ( signal < 0 )
	{
		return ( AHIE_NOMEM );
	}

	pcmBytes = audio->ahiac_MaxBuffSamples * play->Format.Channels * 2;
#ifndef BLUETOOTH_AHI
	pcmBytes *= play->WriteDepth;
#endif

	play->Mix = AllocVecTags( audio->ahiac_BuffSize, TAG_END );

	play->PCM = AllocVecTags( pcmBytes, TAG_END );

	play->Parent = FindTask( NULL );
	play->StartMask = 1UL << signal;
	play->Started = FALSE;
	process = NULL;
	ready = FALSE;

	if ( ( play->Mix ) && ( play->PCM ) )
	{
		process = play->DOS->CreateNewProcTags(
			NP_Entry, Playback_Entry,
			NP_Name, WORKER_NAME,
			NP_UserData, play,
			TAG_END );
	}

	if ( process )
	{
		Wait( play->StartMask );

#ifdef BLUETOOTH_AHI
		ready = WaitBluetoothStartup( play );
#else
		ready = play->Started;
#endif
	}

	if ( ! ready )
	{
		AHI_Stop( self, AHISF_PLAY, audio );

		SetSignal( 0, play->StartMask );

		FreeSignal( signal );

#ifdef BLUETOOTH_AHI
		return ( process ? AHIE_UNKNOWN : AHIE_NOMEM );
#else
		return ( AHIE_NOMEM );
#endif
	}

	FreeSignal( signal );

	return ( AHIE_OK );
}

// --

static void
AHI_Free( struct AHIsubIFace *self, struct AHIAudioCtrlDrv *audio )
{
	struct Playback *play;

	play = audio->ahiac_DriverData;

	if ( ! play )
	{
		return;
	}

	AHI_Stop( self, AHISF_PLAY, audio );

#ifdef BLUETOOTH_AHI
	CloseBluetoothPlayback( play );
#endif

	if ( play->Request )
	{
		if ( play->Request->io_Device )
		{
			CloseDevice( (APTR) play->Request );
		}

		FreeSysObject( ASOT_IOREQUEST, play->Request );
	}

	if ( play->CallerPort )
	{
		FreeSysObject( ASOT_PORT, play->CallerPort );
	}

	if ( play->Utility )
	{
		DropInterface( (APTR) play->Utility );
	}

	if ( play->UtilityBase )
	{
		CloseLibrary( play->UtilityBase );
	}

	if ( play->DOS )
	{
		DropInterface( (APTR) play->DOS );
	}

	if ( play->DOSBase )
	{
		CloseLibrary( play->DOSBase );
	}

	audio->ahiac_DriverData = NULL;

	FreeVec( play );
}

// --

#ifndef BLUETOOTH_AHI
static BOOL
OpenUSBPlayback( struct Playback *play )
{
	struct IOStdReq *io;

	ULONG unit;

	io = play->Request;

	for( unit = AUDIO_PLAYBACK_UNIT_BASE; unit < AUDIO_MAX_UNITS; unit++ )
	{
		if ( OpenDevice( AUDIO_DEVICE_NAME, unit, (APTR) io, 0 ) )
		{
			io->io_Device = NULL;

			continue;
		}

		io->io_Command = AUDIO_CMD_QUERY;
		io->io_Data = & play->Format;
		io->io_Length = sizeof( play->Format );

		if ( ( ! DoIO( (APTR) io ) ) && ( io->io_Actual == sizeof( play->Format ) ) &&
			( play->Format.Encoding == AUDIO_ENCODING_PCM_S16LE ) &&
			( ( play->Format.Channels == 1 ) || ( play->Format.Channels == 2 ) ) )
		{
			play->WriteDepth = ( io->io_Device->dd_Library.lib_Version > 1 ||
				( ( io->io_Device->dd_Library.lib_Version == 1 ) &&
				  ( io->io_Device->dd_Library.lib_Revision >= 1 ) ) ) ? 2 : 1;

			return ( TRUE );
		}

		CloseDevice( (APTR) io );

		io->io_Device = NULL;
	}

	return ( FALSE );
}
#endif

// --

static ULONG
AHI_Alloc( struct AHIsubIFace *self, struct TagItem *tags UNUSED, struct AHIAudioCtrlDrv *audio )
{
	struct Playback *play;

	AHI_Free( self, audio );

	play = AllocVecTags( sizeof( *play ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! play )
	{
		return ( AHISF_ERROR );
	}

	audio->ahiac_DriverData = play;
	play->Audio = audio;

	InitSemaphore( & play->Lifetime );

	play->DOSBase = OpenLibrary( "dos.library", 53 );

	play->UtilityBase = OpenLibrary( "utility.library", 50 );

	if ( ( ! play->DOSBase ) || ( ! play->UtilityBase ) )
	{
		goto failed;
	}

	play->DOS = (APTR) GetInterface( play->DOSBase, "main", 1, NULL );

	play->Utility = (APTR) GetInterface( play->UtilityBase, "main", 1, NULL );

	play->CallerPort = AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ( ! play->DOS ) || ( ! play->Utility ) || ( ! play->CallerPort ) )
	{
		goto failed;
	}

	play->Request = AllocSysObjectTags( ASOT_IOREQUEST, ASOIOR_Size, sizeof( struct IOStdReq ),
		ASOIOR_ReplyPort, play->CallerPort, TAG_END );

	if ( ! play->Request )
	{
		goto failed;
	}

#ifdef BLUETOOTH_AHI
	if ( ! OpenBluetoothPlayback( play ) )
	{
		goto failed;
	}
#else
	if ( ! OpenUSBPlayback( play ) )
	{
		goto failed;
	}

#endif

	audio->ahiac_MixFreq = play->Format.SampleRate;

	return ( AHISF_MIXING | AHISF_TIMING | AHISF_KNOWSTEREO );

failed:
	AHI_Free( self, audio );

	return ( AHISF_ERROR );
}

// --

static LONG
AHI_GetAttr( struct AHIsubIFace *self UNUSED, ULONG attribute, LONG argument,
	LONG fallback, struct TagItem *tags UNUSED, struct AHIAudioCtrlDrv *audio )
{
	struct Playback *play;
	ULONG rate;

	play = audio ? audio->ahiac_DriverData : NULL;
	rate = play ? play->Format.SampleRate : DEFAULT_RATE;

	switch( attribute )
	{
		case AHIDB_Bits: return 16;
		case AHIDB_MaxChannels: return 128;
		case AHIDB_Frequencies: return 1;
		case AHIDB_Frequency: return argument == 0 ? (LONG) rate : fallback;
		case AHIDB_Index: return 0;
		case AHIDB_MinMixFreq:
		case AHIDB_MaxMixFreq: return rate;
		case AHIDB_Record:
		case AHIDB_FullDuplex:
		case AHIDB_Realtime: return FALSE;
		case AHIDB_Outputs: return 1;
		case AHIDB_Output: return argument == 0 ? (LONG) OUTPUT_NAME : fallback;
		case AHIDB_Version: return (LONG) DRIVER_ID;
	}

	return ( fallback );
}

// --

static void
AHI_Disable( struct AHIsubIFace *self UNUSED, struct AHIAudioCtrlDrv *audio UNUSED )
{
	Forbid();
}
static void
AHI_Enable( struct AHIsubIFace *self UNUSED, struct AHIAudioCtrlDrv *audio UNUSED )
{
	Permit();
}
static ULONG
AHI_Update( struct AHIsubIFace *self UNUSED, ULONG flags UNUSED, struct AHIAudioCtrlDrv *audio UNUSED )
{
	return ( AHIE_OK );
}
static ULONG
AHI_SetVol( struct AHIsubIFace *s UNUSED, UWORD c UNUSED, Fixed v UNUSED, sposition p UNUSED, struct AHIAudioCtrlDrv *a UNUSED, ULONG f UNUSED )
{
	return ( AHIS_UNKNOWN );
}
static ULONG
AHI_SetFreq( struct AHIsubIFace *s UNUSED, UWORD c UNUSED, ULONG r UNUSED, struct AHIAudioCtrlDrv *a UNUSED, ULONG f UNUSED )
{
	return ( AHIS_UNKNOWN );
}
static ULONG
AHI_SetSound( struct AHIsubIFace *s UNUSED, UWORD c UNUSED, UWORD n UNUSED, ULONG o UNUSED, LONG l UNUSED, struct AHIAudioCtrlDrv *a UNUSED, ULONG f UNUSED )
{
	return ( AHIS_UNKNOWN );
}
static ULONG
AHI_SetEffect( struct AHIsubIFace *s UNUSED, APTR e UNUSED, struct AHIAudioCtrlDrv *a UNUSED )
{
	return ( AHIS_UNKNOWN );
}
static ULONG
AHI_LoadSound( struct AHIsubIFace *s UNUSED, UWORD n UNUSED, ULONG t UNUSED, APTR i UNUSED, struct AHIAudioCtrlDrv *a UNUSED )
{
	return ( AHIS_UNKNOWN );
}
static ULONG
AHI_UnloadSound( struct AHIsubIFace *s UNUSED, UWORD n UNUSED, struct AHIAudioCtrlDrv *a UNUSED )
{
	return ( AHIS_UNKNOWN );
}
static LONG
AHI_HardwareControl( struct AHIsubIFace *s UNUSED, ULONG t UNUSED, LONG v UNUSED, struct AHIAudioCtrlDrv *a UNUSED )
{
	return ( 0 );
}

// --

struct AHIBase
{
	struct Library Library;
	BPTR SegList;
};

static uint32
Interface_Obtain( struct Interface *self )
{
	return ( self->Data.RefCount++ );
}

static uint32
Interface_Release( struct Interface *self )
{
	return ( self->Data.RefCount-- );
}

static struct Library *
Library_Open( struct LibraryManagerInterface *self, ULONG version )
{
	struct Library *base;

	base = self->Data.LibBase;

	if ( version > base->lib_Version )
	{
		return ( NULL );
	}

	base->lib_OpenCnt++;
	base->lib_Flags &= ~LIBF_DELEXP;

	return ( base );
}

static BPTR
Library_Expunge( struct LibraryManagerInterface *self )
{
	struct AHIBase *base;
	BPTR seglist;

	base = (APTR) self->Data.LibBase;

	if ( base->Library.lib_OpenCnt )
	{
		base->Library.lib_Flags |= LIBF_DELEXP;

		return ( 0 );
	}

	seglist = base->SegList;

	Remove( & base->Library.lib_Node );

	DeleteLibrary( & base->Library );

	return ( seglist );
}

static BPTR
Library_Close( struct LibraryManagerInterface *self )
{
	struct Library *base;

	base = self->Data.LibBase;
	base->lib_OpenCnt--;

	if ( ( ! base->lib_OpenCnt ) && ( base->lib_Flags & LIBF_DELEXP ) )
	{
		return ( Library_Expunge( self ) );
	}

	return ( 0 );
}

static struct AHIBase *
Library_Init( struct AHIBase *base, BPTR seglist, struct ExecIFace *exec )
{
	IExec = exec;
	base->SegList = seglist;
	base->Library.lib_Node.ln_Type = NT_LIBRARY;
	base->Library.lib_Node.ln_Name = DRIVER_NAME;
	base->Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
	base->Library.lib_Version = 6;
	base->Library.lib_Revision = 1;
	base->Library.lib_IdString = DRIVER_ID;

	return ( base );
}

static const APTR ManagerVectors[] =
{
	Interface_Obtain, Interface_Release, NULL, NULL,
	Library_Open, Library_Close, Library_Expunge, NULL, (APTR) -1
};

static const APTR AHIVectors[] =
{
	Interface_Obtain, Interface_Release, NULL, NULL,
	AHI_Alloc, AHI_Free, AHI_Disable, AHI_Enable, AHI_Start, AHI_Update, AHI_Stop,
	AHI_SetVol, AHI_SetFreq, AHI_SetSound, AHI_SetEffect, AHI_LoadSound, AHI_UnloadSound,
	AHI_GetAttr, AHI_HardwareControl, (APTR) -1
};

static const struct TagItem ManagerTags[] =
{
	{ MIT_Name, (Tag) "__library" },
	{ MIT_VectorTable, (Tag) ManagerVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

static const struct TagItem AHITags[] =
{
	{ MIT_Name, (Tag) "main" },
	{ MIT_VectorTable, (Tag) AHIVectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};

static const APTR Interfaces[] = { (APTR) ManagerTags, (APTR) AHITags, NULL };

static const struct TagItem InitTags[] =
{
	{ CLT_DataSize, sizeof( struct AHIBase ) },
	{ CLT_Interfaces, (Tag) Interfaces },
	{ CLT_InitFunc, (Tag) Library_Init },
	{ CLT_NoLegacyIFace, TRUE },
	{ TAG_END, 0 }
};

static const USED char VersionTag[] = "\0$VER: " DRIVER_ID;

static const USED struct Resident Resident =
{
	RTC_MATCHWORD, (APTR) & Resident, (APTR) ( & Resident + 1 ),
	RTF_NATIVE | RTF_AUTOINIT, 6, NT_LIBRARY, 0,
	DRIVER_NAME, DRIVER_ID, (APTR) InitTags
};

int
_start( void )
{
	return ( RETURN_FAIL );
}

// --
