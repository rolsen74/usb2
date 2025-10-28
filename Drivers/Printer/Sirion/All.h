
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

// --

#ifndef PRT_DEVICE_H
#define PRT_DEVICE_H

// --

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <devices/newstyle.h>
#include <devices/parallel.h>
#include <dos/dostags.h>

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

// --

#include "Types.h"
#include "Version.h"
// --
#include "Base.h"
#include "Task.h"
#include "Debug.h"
#include "Buffer.h"
#include "Config.h"
#include "Printer.h"

// --

// --

extern struct Interface *INewlib;
extern struct Library *NewlibBase;

void	Cmd_Write_Buffer(		struct PrinterUnit *unit, struct IOExtPar *pario );
void	Cmd_Write_Direct(		struct PrinterUnit *unit, struct IOExtPar *pario );
S32		Resources_Init(			struct PrinterBase *devBase );
void	Resources_Free(			struct PrinterBase *devBase );
void	Buffer_Flush(			struct PrinterUnit *unit );
S32		Buffer_Write(			struct PrinterUnit *unit, U8 *data, U32 size );
void	Buffer_Schedule(		struct PrinterUnit *unit );
void	Direct_Schedule(		struct PrinterUnit *unit );
S32		Process_Find_Device(	struct PrinterUnit *unit );
S32		Process_Setup_Device(	struct PrinterUnit *unit );

// --

#endif
