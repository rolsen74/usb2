
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef WIRED_DEBUG_H
#define WIRED_DEBUG_H

#include <proto/exec.h>
#include "Types.h"

// --

#ifdef DO_DEBUG
#define SHOWMSG(...) do { IExec->DebugPrintF( __VA_ARGS__ ); IExec->DebugPrintF( "\n" ); } while ( 0 )
#else
#define SHOWMSG(...) ((void) 0)
#endif

#endif
