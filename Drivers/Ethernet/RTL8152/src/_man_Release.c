
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

U32
Device_Release( struct DeviceManagerInterface *Self )
{
    Self->Data.RefCount--;

    return( Self->Data.RefCount );
}

// --
