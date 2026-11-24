
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/SmartCard.h"

// --

static LONG
GetReader( struct USB2SmartCardIFace *Self, ULONG unit, struct USB2SmartCardInfo *info, ULONG *instance )
{
	struct SmartCardDriver *base;
	struct SmartCardReader *reader;

	LONG result;

	if ( ( unit >= USB2SMARTCARD_UNITS ) || ( ! info ) )
	{
		return ( USB2SC_BadArgument );
	}

	base = (PTR) Self->Data.LibBase;

	IExec->ObtainSemaphore( & base->Lock );

	reader = base->Readers[unit];
	result = USB2SC_NoReader;

	if ( reader )
	{
		*info = reader->Info;

		if ( instance )
		{
			*instance = reader->Instance;
		}

		result = USB2SC_Okay;
	}

	IExec->ReleaseSemaphore( & base->Lock );

	return ( result );
}

// --

LONG
SmartCard_GetInfo( struct USB2SmartCardIFace *Self, ULONG unit, struct USB2SmartCardInfo *info )
{
	return ( GetReader( Self, unit, info, NULL ) );
}

// --

LONG
SmartCard_GetInstance( struct USB2SmartCardIFace *Self, ULONG unit, struct USB2SmartCardInfo *info, ULONG *instance )
{
	if ( ! instance )
	{
		return ( USB2SC_BadArgument );
	}

	*instance = 0;

	return ( GetReader( Self, unit, info, instance ) );
}

// --

static S32
IsShortAPDU( const U8 *data, ULONG size )
{
	ULONG length;

	if ( ( ! data ) || ( size < 4 ) || ( size > USB2SMARTCARD_APDU_MAX ) )
	{
		return ( FALSE );
	}

	if ( size <= 5 )
	{
		return ( TRUE );
	}

	length = data[4];

	return ( ( length ) && ( ( size == 5 + length ) || ( size == 6 + length ) ) );
}

// --

static LONG
Command( struct USB2SmartCardIFace *Self, ULONG unit, ULONG instance, ULONG slot,
	enum USB2SmartCardOperation operation, CONST_APTR input, ULONG inputSize,
	APTR output, ULONG outputSize, struct USB2SmartCardReply *reply, BOOL checkInstance )
{
	struct SmartCardDriver *base;
	struct SmartCardReader *reader;
	struct SmartCardCommand *command;
	struct MsgPort *port;

	ULONG required;
	LONG result;
	S32 queued;

	if ( ! reply )
	{
		return ( USB2SC_BadArgument );
	}

	reply->Length = 0;
	reply->State = USB2SC_Unknown;
	reply->ReaderError = 0;

	if ( ( unit >= USB2SMARTCARD_UNITS ) || ( (U32) operation > USB2SCC_Exchange ) ||
		( ( checkInstance ) && ( ! instance ) ) ||
		( ( outputSize ) && ( ! output ) ) || ( ( operation != USB2SCC_Exchange ) && ( inputSize ) ) ||
		( ( operation == USB2SCC_Exchange ) && ( ! IsShortAPDU( input, inputSize ) ) ) )
	{
		return ( USB2SC_BadArgument );
	}

	required = 0;

	if ( operation == USB2SCC_PowerOn )
	{
		required = USB2SMARTCARD_ATR_MAX;
	}
	else if ( operation == USB2SCC_Exchange )
	{
		required = USB2SMARTCARD_APDU_RESPONSE_MAX;
	}

	if ( outputSize < required )
	{
		reply->Length = required;

		return ( USB2SC_BufferTooSmall );
	}

	port = IExec->AllocSysObjectTags( ASOT_PORT, TAG_END );

	if ( ! port )
	{
		return ( USB2SC_NoMemory );
	}

	command = IExec->AllocVecTags( sizeof( *command ), AVT_Type, MEMF_SHARED, AVT_Clear, 0, TAG_END );

	if ( ! command )
	{
		IExec->FreeSysObject( ASOT_PORT, port );

		return ( USB2SC_NoMemory );
	}

	command->Message.mn_ReplyPort = port;
	command->Message.mn_Length = sizeof( *command );
	command->Reply.State = USB2SC_Unknown;
	command->InputSize = inputSize;
	command->Slot = slot;
	command->Operation = operation;
	command->Result = USB2SC_NoReader;
	queued = FALSE;

	if ( inputSize )
	{
		IExec->CopyMem( input, command->Input, inputSize );
	}

	base = (PTR) Self->Data.LibBase;

	IExec->ObtainSemaphore( & base->Lock );

	reader = base->Readers[unit];

	if ( ( reader ) && ( ( ! checkInstance ) || ( reader->Instance == instance ) ) )
	{
		if ( slot < reader->Info.Slots )
		{
			IExec->PutMsg( reader->Commands, & command->Message );

			queued = TRUE;
		}
		else
		{
			command->Result = USB2SC_BadArgument;
		}
	}

	IExec->ReleaseSemaphore( & base->Lock );

	if ( queued )
	{
		IExec->WaitPort( port );

		IExec->GetMsg( port );

		*reply = command->Reply;

		if ( ( command->Result == USB2SC_Okay ) && ( reply->Length ) )
		{
			IExec->CopyMem( command->Output, output, reply->Length );
		}
	}

	IExec->FreeSysObject( ASOT_PORT, port );

	result = command->Result;

	IExec->FreeVec( command );

	return ( result );
}

// --

LONG
SmartCard_Command( struct USB2SmartCardIFace *Self, ULONG unit, ULONG slot,
	enum USB2SmartCardOperation operation, CONST_APTR input, ULONG inputSize,
	APTR output, ULONG outputSize, struct USB2SmartCardReply *reply )
{
	return ( Command( Self, unit, 0, slot, operation, input, inputSize, output, outputSize, reply, FALSE ) );
}

// --

LONG
SmartCard_CommandInstance( struct USB2SmartCardIFace *Self, ULONG unit, ULONG instance, ULONG slot,
	enum USB2SmartCardOperation operation, CONST_APTR input, ULONG inputSize,
	APTR output, ULONG outputSize, struct USB2SmartCardReply *reply )
{
	return ( Command( Self, unit, instance, slot, operation, input, inputSize, output, outputSize, reply, TRUE ) );
}

// --
