
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "EthDevice.h"

// --

SEC_CODE static void
CopyName( char *target, const char *source )
{
	U32 index;

	if ( source )
	{
		for( index = 0; ( index < USB2ETH_TITLE_SIZE - 1 ) && ( source[index] ); index++ )
		{
			target[index] = source[index];
		}
	}
}

// --

SEC_CODE static struct USB2EthUnit *
Register( struct USB2EthIFace *Self, const struct USB2EthOps *ops, PTR adapter, ULONG *number, ULONG type,
	const char *title, const char *driver, const char *alias, const struct USB2_DriverMessage *msg );

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
	return ( EthDevice_RegisterNamedAdapter( Self, ops, adapter, number, type, NULL ) );
}

// --

SEC_CODE struct USB2EthUnit *
EthDevice_RegisterNamedAdapter( struct USB2EthIFace *Self, const struct USB2EthOps *ops, PTR adapter,
	ULONG *number, ULONG type, const char *title )
{
	return ( EthDevice_RegisterClassAdapter( Self, ops, adapter, number, type, title, NULL, NULL ) );
}

// --

SEC_CODE struct USB2EthUnit *
EthDevice_RegisterClassAdapter( struct USB2EthIFace *Self, const struct USB2EthOps *ops, PTR adapter,
	ULONG *number, ULONG type, const char *title, const char *driver, const char *alias )
{
	return ( Register( Self, ops, adapter, number, type, title, driver, alias, NULL ) );
}

// --

SEC_CODE struct USB2EthUnit *
EthDevice_RegisterDevice( struct USB2EthIFace *Self, const struct USB2EthOps *ops, PTR adapter,
	ULONG *number, ULONG type, const struct USB2_DriverMessage *msg )
{
	if ( ! msg )
	{
		return ( NULL );
	}

	return ( Register( Self, ops, adapter, number, type, msg->ClassTitle, msg->ClassDriver, msg->ClassAlias, msg ) );
}

// --

SEC_CODE static struct USB2EthUnit *
Register( struct USB2EthIFace *Self, const struct USB2EthOps *ops, PTR adapter, ULONG *number, ULONG type,
	const char *title, const char *driver, const char *alias, const struct USB2_DriverMessage *msg )
{
	struct USB2EthDevice *base;
	struct USB2EthUnit *unit;
	struct ExecIFace *IExec;
	struct USB2UnitIdentity identity;
	struct USB2UnitSearch search;

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

	unit->Info.Type = type;

	CopyName( unit->Info.Title, title );

	CopyName( unit->Info.Driver, driver );

	CopyName( unit->Info.Alias, alias );

	USB2_ReadUnitIdentity( &identity, msg, 0 );

	USB2_InitUnitSearch( &search, USB2_ETH_MAX_UNITS );

	IExec->ObtainSemaphore( & base->OpenLock );

	IExec->ObtainSemaphore( & base->RouteLock );

	for ( index = 0; index < USB2_ETH_MAX_UNITS; index++ )
	{
		if ( ! base->Units[index] )
		{
			USB2_ConsiderUnit( &search, &base->Identity[index], &identity, index, base->Reserved[index] != 0 );
		}
	}

	index = USB2_SelectUnit( &search );

	if ( index < USB2_ETH_MAX_UNITS )
	{
		IExec->CopyMem( &identity, &base->Identity[index], sizeof( identity ) );

		unit->Ops = ops;
		unit->Adapter = adapter;
		unit->Number = index;

		if ( ! ++base->NextRegistrationID )
		{
			base->NextRegistrationID++;
		}

		unit->RegistrationID = base->NextRegistrationID;
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
