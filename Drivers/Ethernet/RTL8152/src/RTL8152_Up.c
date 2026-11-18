
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Controller.h"

// --

void
RTL8152_Up( struct devUnit *unit )
{

    RTL8152_Disable_ALDPS( unit );

    RTL8152_Exit_OOB( unit );

    RTL8152_Enable_ALDPS( unit );

}

// --
