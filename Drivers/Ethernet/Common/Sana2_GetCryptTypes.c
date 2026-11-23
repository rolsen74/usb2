
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include <interfaces/exec.h>
#include "Sana2Commands.h"

// --

extern struct ExecIFace *IExec;

// --

void
Sana2_GetCryptTypes( struct IOSana2Req *ioreq, const uint8 *types, uint32 count )
{
	uint8 *result;

	ioreq->ios2_DataLength = 0;
	ioreq->ios2_StatData = NULL;

	if ( ! ioreq->ios2_Data )
	{
		ioreq->ios2_Req.io_Error = S2ERR_BAD_ARGUMENT;
		ioreq->ios2_WireError = S2WERR_NULL_POINTER;

		return;
	}

	if ( ! count )
	{
		return;
	}

	result = IExec->AllocPooled( ioreq->ios2_Data, count );

	if ( ! result )
	{
		ioreq->ios2_Req.io_Error = S2ERR_NO_RESOURCES;

		return;
	}

	IExec->CopyMem( types, result, count );

	ioreq->ios2_DataLength = count;
	ioreq->ios2_StatData = result;
}

// --
