
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
RTL8152_Enable_ALDPS( struct devUnit *unit )
{
	myWriteReg( unit, URE_OCP_ALDPS_CONFIG, URE_ENPWRSAVE | URE_ENPDNPS | URE_LINKENA | URE_DIS_SDSAVE );

}

// --
