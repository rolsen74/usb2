
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/SmartCard.h"

// --

LONG
SmartCard_GetInfo( struct USB2SmartCardIFace *Self, ULONG unit, struct USB2SmartCardInfo *info )
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
		result = USB2SC_Okay;
	}

	IExec->ReleaseSemaphore( & base->Lock );

	return ( result );
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

LONG
SmartCard_Command( struct USB2SmartCardIFace *Self, ULONG unit, ULONG slot,
	enum USB2SmartCardOperation operation, CONST_APTR input, ULONG inputSize,
	APTR output, ULONG outputSize, struct USB2SmartCardReply *reply )
{
	struct SmartCardDriver *base;
	struct SmartCardReader *reader;
	struct SmartCardCommand *command;
	struct MsgPort *port;
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
		( ( outputSize ) && ( ! output ) ) || ( ( operation != USB2SCC_Exchange ) && ( inputSize ) ) ||
		( ( operation == USB2SCC_Exchange ) && ( ! IsShortAPDU( input, inputSize ) ) ) )
	{
		return ( USB2SC_BadArgument );
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
	command->Reply = reply;
	command->Input = input;
	command->Output = output;
	command->InputSize = inputSize;
	command->OutputSize = outputSize;
	command->Slot = slot;
	command->Operation = operation;
	command->Result = USB2SC_NoReader;
	queued = FALSE;
	base = (PTR) Self->Data.LibBase;

	IExec->ObtainSemaphore( & base->Lock );

	reader = base->Readers[unit];

	if ( reader )
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
	}

	IExec->FreeSysObject( ASOT_PORT, port );

	result = command->Result;

	IExec->FreeVec( command );

	return ( result );
}

// --
