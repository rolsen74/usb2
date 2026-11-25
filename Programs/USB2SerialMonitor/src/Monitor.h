
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_SERIAL_MONITOR_H
#define USB2_SERIAL_MONITOR_H

#define DEVICES_SERIAL_H_OBSOLETE
#include <devices/serial.h>
#include <devices/timer.h>
#include <devices/usb2.h>
#include <interfaces/usb2port.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

// --

#define CAPTURE_LIMIT ( 8U * 1024U * 1024U )
#define PREVIEW_LIMIT 4096U
#define READ_SIZE 1024U

enum MonitorClass
{
	CLASS_WINDOW,
	CLASS_LAYOUT,
	CLASS_BUTTON,
	CLASS_INTEGER,
	CLASS_CHOOSER,
	CLASS_LABEL,
	CLASS_EDITOR,
	CLASS_SCROLLER,
	CLASS_STRING,
	CLASS_COUNT
};

enum MonitorDialog
{
	DIALOG_SETUP = 1,
	DIALOG_SAVE
};

struct LineSettings
{
	U32 Unit;
	U32 Baud;
	U32 DataBits;
	U32 StopBits;
	U32 Parity;
};

struct Monitor
{
	struct ClassLibrary *Bases[CLASS_COUNT];
	struct IClass *Classes[CLASS_COUNT];
	struct MsgPort *SerialPort;
	struct MsgPort *TimerPort;
	struct IOExtSer *Serial;
	struct TimeRequest *Timer;
	struct Window *Window;
	Object *Main;
	Object *Editor;
	Object *Status;
	Object *CaptureButton;
	Object *Dialog;
	Object *Fields[5];
	enum MonitorDialog DialogKind;

	struct LineSettings Settings;

	U8 *Data;
	U32 Length;
	U32 Overruns;
	U8 Input[READ_SIZE];
	char Preview[PREVIEW_LIMIT + 1];
	char StatusText[128];
	char SavePath[1024];
	S32 Error;
	const char *Failure;
	const char *Notice;
	BOOL Capturing;
	BOOL Pending;
	BOOL Dirty;
	BOOL Run;
};

S32 Capture_Start( struct Monitor *monitor, const struct LineSettings *settings );
void Capture_Stop( struct Monitor *monitor );
void Capture_Handle( struct Monitor *monitor );
void Capture_Refresh( struct Monitor *monitor );
BOOL Monitor_Save( struct Monitor *monitor, const char *path );
BOOL Monitor_OpenMain( struct Monitor *monitor );
void Monitor_HandleWindows( struct Monitor *monitor );
void Monitor_CloseDialog( struct Monitor *monitor );
U32 Monitor_Signals( struct Monitor *monitor );

#endif
