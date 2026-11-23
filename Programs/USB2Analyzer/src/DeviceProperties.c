
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"

// --

static void
Add_Property_Row( PTR target, U32 level, S32 heading, const char *field, const char *size,
	const char *value, const char *description )
{
	struct Node *row;

	row = AllocListBrowserNode( 4,
		LBNA_Generation, level + 1,
		LBNA_Flags, heading ? LBFLG_HASCHILDREN | LBFLG_SHOWCHILDREN : LBFLG_READONLY,
		LBNA_Column, 0,
		 LBNCA_CopyText, TRUE,
		 LBNCA_Text, field,
		LBNA_Column, 1,
		 LBNCA_CopyText, TRUE,
		 LBNCA_Text, size,
		 LBNCA_Justification, LCJ_CENTER,
		LBNA_Column, 2,
		 LBNCA_CopyText, TRUE,
		 LBNCA_Text, value,
		LBNA_Column, 3,
		 LBNCA_CopyText, TRUE,
		 LBNCA_Text, description,
		TAG_END
	);

	if ( row )
	{
		AddTail( target, row );
	}
}

// --

void
Build_Device_Properties( struct List *list, const struct USB2_FktNode *node )
{
	Describe_Descriptors( node, Add_Property_Row, list );
}

// --
