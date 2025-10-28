
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

struct MsgPort *		USB2MsgPort		= NULL;
struct IORequest *		USB2IOReq		= NULL;
struct Device *			USB2Base		= NULL;
struct USB2_IFace *		IUSB2			= NULL;

// --
