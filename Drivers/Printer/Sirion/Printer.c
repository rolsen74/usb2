
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#include "All.h"

// --

struct Library *		DOSBase			= NULL;
struct DOSIFace *		IDOS			= NULL;

struct Library *		SysBase			= NULL;
struct ExecIFace *		IExec			= NULL;

struct Library *		NewlibBase		= NULL;
struct Interface *		INewlib			= NULL;

struct Library *		UtilityBase		= NULL;
struct UtilityIFace *	IUtility		= NULL;

// --


// --
