
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

APTR
Device_Expunge( struct DeviceManagerInterface *Self )
{
struct devBase *devBase;
APTR retval;


	devBase = (APTR) Self->Data.LibBase;

	if ( devBase->dev_Base.dd_Library.lib_OpenCnt == 0 )
    {
		retval = devBase->dev_SegList;

        /* Undo what the init code did */

		CloseResources( devBase );

        /* Remove Device from Public */

		IExec->Remove( (APTR) devBase );
		IExec->DeleteLibrary( (APTR) devBase );

        IExec->Release();
        IExec = NULL;
    }
    else
    {
		devBase->dev_Base.dd_Library.lib_Flags |= LIBF_DELEXP;
		retval = NULL;
    }

	return( retval );
}

// --
