
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"
#include "Radio.h"

// --

void
RTL8811AU_AccountType( struct EthernetUnit *unit, U32 type, U32 length, S32 receive, S32 dropped )
{
	struct BufferManagement *opener;

	for( opener = (PTR) GetHead( & unit->unit_Openers ); opener; opener = (PTR) GetSucc( & opener->bm_Node ) )
	{
		Sana2_AccountType( & opener->bm_TrackedTypes, type, length, receive, dropped );
	}
}

// --
