
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "EthDevice.h"

// --

SEC_CODE struct USB2EthUnit *
EthDevice_Register( struct USB2EthIFace *Self, const struct USB2EthOps *ops, PTR adapter, ULONG *number )
{
	return ( EthDevice_RegisterAdapter( Self, ops, adapter, number, USB2ETH_ADAPTER_UNKNOWN ) );
}

// --

SEC_CODE struct USB2EthUnit *
EthDevice_RegisterAdapter( struct USB2EthIFace *Self, const struct USB2EthOps *ops, PTR adapter, ULONG *number, ULONG type )
{
	struct USB2EthDevice *base;
	struct USB2EthUnit *unit;
	struct ExecIFace *IExec;

	U32 index;

	if ( ( ! ops ) || ( ! ops->Commands ) || ( ! ops->Open ) || ( ! ops->Close ) ||
		( ! ops->Begin ) || ( ! ops->Abort ) || ( ! ops->Destroy ) || ( ! adapter ) || ( ! number ) )
	{
		return ( NULL );
	}

	base = (PTR) Self->Data.LibBase;
	IExec = base->USB->usb_IExec;

	unit = IExec->AllocVecTags( sizeof( *unit ), AVT_Clear, 0, TAG_END );

	if ( ! unit )
	{
		return ( NULL );
	}

	IExec->ObtainSemaphore( & base->OpenLock );

	IExec->ObtainSemaphore( & base->RouteLock );

	for( index = 0; index < USB2_ETH_MAX_UNITS; index++ )
	{
		if ( ! base->Units[index] )
		{
			break;
		}
	}

	if ( index < USB2_ETH_MAX_UNITS )
	{
		unit->Ops = ops;
		unit->Adapter = adapter;
		unit->Number = index;
		unit->Type = type;
		*number = index;
		base->Units[index] = unit;
	}
	else
	{
		IExec->FreeVec( unit );

		unit = NULL;
	}

	IExec->ReleaseSemaphore( & base->RouteLock );

	IExec->ReleaseSemaphore( & base->OpenLock );

	return ( unit );
}

// --
