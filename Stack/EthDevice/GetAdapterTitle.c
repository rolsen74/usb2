
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "EthDevice.h"

// --

SEC_CODE ULONG
EthDevice_GetAdapterTitle( struct USB2EthIFace *Self, ULONG number, char *title )
{
	struct USB2EthDevice *base;
	struct USB2EthUnit *unit;
	struct ExecIFace *IExec;

	ULONG type;

	type = USB2ETH_ADAPTER_UNKNOWN;

	if ( ! title )
	{
		return ( type );
	}

	title[0] = 0;

	if ( number >= USB2_ETH_MAX_UNITS )
	{
		return ( type );
	}

	base = (PTR) Self->Data.LibBase;
	IExec = base->USB->usb_IExec;

	IExec->ObtainSemaphore( & base->RouteLock );

	unit = base->Units[number];

	if ( ( unit ) && ( ! unit->Detached ) )
	{
		type = unit->Info.Type;

		IExec->CopyMem( unit->Info.Title, title, sizeof( unit->Info.Title ) );
	}

	IExec->ReleaseSemaphore( & base->RouteLock );

	return ( type );
}

// --
