
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_ALL_H
#error Include "inc/All.h" first
#endif

#ifndef INC_EXTERNALS_H
#define INC_EXTERNALS_H

// --
// MainScreen.c

extern struct Screen *	Main_ScreenAdr;
extern enum SCRTYPE		Main_ScreenType;

// --
// MainWindow.c
extern Object *			Main_WinObject;
extern U32				Main_WindowBits;
extern S32				Main_WinXPos;
extern S32				Main_WinYPos;
extern S32				Main_WinWidth;
extern S32				Main_WinHeight;
extern struct List		DeviceTreeUSBList;
extern struct List		DeviceTreeGUIList;
extern struct List		PropertiesGUIList;
extern struct List		CaptureGUIList;
extern struct List		IORequestGUIList;

// --
// Prefs.c
extern struct ScreenPrefs Prefs_Screen;
extern U32 Prefs_WindowBits;

// --
// USB.c
extern U32				USBNotifyBit;

// --
// USB2Analyzer.c
extern S32				Restart;
extern S32				Running;

#endif
