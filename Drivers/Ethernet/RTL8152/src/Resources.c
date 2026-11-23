
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "All.h"

// --

struct Library *SysBase;

struct ExecIFace *			IExec			= NULL;

struct Library *			NewlibBase		= NULL;
struct Interface *			INewlib			= NULL;

struct Library *			DOSBase			= NULL;
struct DOSIFace *			IDOS			= NULL;

struct Library *			UtilityBase		= NULL;
struct UtilityIFace *		IUtility		= NULL;

struct TimerIFace *			ITimer			= NULL;

// --
