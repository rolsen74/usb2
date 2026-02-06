
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "CompPro.h"

// --
// -- AmigaInput Interface

static const PTR AI_Vectors[] =
{
	(PTR) _AI_Obtain,
	(PTR) _AI_Release,
	(PTR) NULL,
	(PTR) NULL,
	(PTR) _AI_InitDriver,
	(PTR) _AI_SetEventMode,
	(PTR) _AI_CleanUp,
	(PTR) _AI_Read,
	(PTR) _AI_Read_KBD,
	(PTR) _AI_TestUnit,
	(PTR) _AI_HandleSettings,
	(PTR) _AI_SetCalibrateMode,
	// rwo
//	(PTR) _AI_GetAttrs,
//	(PTR) _AI_GetAttrsA,
	(PTR) -1
};

const struct TagItem AI_Tags[] =
{
	{ MIT_Name,				(U32) "main"		},
	{ MIT_VectorTable,		(U32) AI_Vectors	},
	{ MIT_Version,			(U32) 1				},
	{ TAG_END,				(U32) 0				}
};

// --
