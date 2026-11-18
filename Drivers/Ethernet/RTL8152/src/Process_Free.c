
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

void
unit_Process_Free( struct devUnit *unit )
{

	unit_Process_FreeUSB( unit );

	unit_Process_CloseUSBDevice( unit );

	unit_Process_FreeMisc( unit );

}

// --
