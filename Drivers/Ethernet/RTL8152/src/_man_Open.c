
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

S32
Device_Open(
    struct DeviceManagerInterface *Self,
    struct IOSana2Req *ioreq,
    U32 unitnum,
    U32 flags UNUSED )
{
struct BufferManagement *bm;
struct TagItem *taglist;
struct devBase *devBase;
struct devUnit *unit;
S32 error;

	devBase = (APTR) Self->Data.LibBase;
	bm = NULL;

	IExec->ObtainSemaphore( & devBase->dev_GlobalLock );

    /* Expunge protection */
	devBase->dev_Base.dd_Library.lib_OpenCnt++;

    /* Check request size */
    if ( ioreq->ios2_Req.io_Message.mn_Length < sizeof( struct IOSana2Req ))
    {
	    SHOWMSG( "Invalid request size" );
        error = IOERR_OPENFAIL;
        goto bailout;
    }

	if ( unitnum >= sizeof( devBase->dev_Units ) / sizeof( devBase->dev_Units[0] ) )
	{
		SHOWMSG( "Invalid unit number" );

		error = IOERR_OPENFAIL;

		goto bailout;
	}

	taglist = ioreq->ios2_BufferManagement;

	bm = IExec->AllocVecTags( sizeof( *bm ), AVT_ClearWithValue, 0, TAG_END );

	if ( ! bm )
	{
		error = S2ERR_NO_RESOURCES;

		goto bailout;
	}

	// Query-only openers may omit the packet callbacks.
	if ( taglist )
	{
		bm->bm_CopyToBuff = (PTR) IUtility->GetTagData( S2_CopyToBuff, 0, taglist );

		bm->bm_CopyFromBuff = (PTR) IUtility->GetTagData( S2_CopyFromBuff, 0, taglist );

		bm->bm_PacketHook = (PTR) IUtility->GetTagData( S2_PacketFilter, 0, taglist );
	}

	unit = devBase->dev_Units[ unitnum ];

	if ( unit )
	{
		if ( ! unit->unit_Running )
		{
			error = IOERR_OPENFAIL;

			goto bailout;
		}

		goto alreadyopen;
	}

	/* Create per unit data */
	unit = (APTR) IExec->AllocVecTags( sizeof( struct devUnit ),
		AVT_ClearWithValue, 0,
		TAG_END
	);

	if ( unit == NULL )
	{
		SHOWMSG( "Error allocating memory" );
		error = IOERR_OPENFAIL;
		goto bailout;
	}

	unit->unit_UnitNumber	= unitnum;

	/* Start the subtask */

	if ( unit_Start( unit ) == FALSE )
	{
		SHOWMSG( "Error stating Unit Process" );
		IExec->FreeVec( unit );
		error = IOERR_OPENFAIL;
		goto bailout;
	}

	devBase->dev_Units[ unitnum ] = unit;

alreadyopen:

	ioreq->ios2_BufferManagement = bm;
	ioreq->ios2_Req.io_Unit = (APTR) unit;
	unit->unit_Base.unit_OpenCnt++;

	/* Prevent delayed expunges */
	devBase->dev_Base.dd_Library.lib_Flags &= ~LIBF_DELEXP;

	/* Mark IORequest as "complete" */
	ioreq->ios2_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;

	/* Success */
	error = 0;

bailout:

    if ( error == 0 )
    {
		devBase->dev_Base.dd_Library.lib_OpenCnt++;
    }
    else
    {
		if ( bm )
		{
			IExec->FreeVec( bm );
		}

		ioreq->ios2_Req.io_Device = NULL;
		ioreq->ios2_Req.io_Unit = NULL;
    }

    ioreq->ios2_Req.io_Error = error;

    /* End Expunge protection */
	devBase->dev_Base.dd_Library.lib_OpenCnt--;

	IExec->ReleaseSemaphore( & devBase->dev_GlobalLock );

    return( error );
}

// --
