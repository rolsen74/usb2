
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
SiS163U_StopRadio( struct EthernetUnit *unit )
{
	return ( SiS163U_WriteRegister( unit->unit_USB_Register->Res_Control, 0, 0x50800000 ) );
}

// --
