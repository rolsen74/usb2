
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_ALL_H
#error Include "inc/All.h" first
#endif

#ifndef INC_BASE_H
#define INC_BASE_H

// --

struct PTPBase
{
	struct Library					lib_Base;
	PTR								lib_SegList;
};

// --

struct PTPData
{
	int	dummy;
	// struct BlankerRender *			bd_BlankerRender;
	// U32								bd_BlankingMode;
	// U32								bd_ScreenMode;
	// enum ScreenType					bd_ScreenType;
	// U32								bd_SliderValue;
	// U32								bd_RefetchSettings;
	// U32								bd_DoScreenSync;
	// struct BlankerPrefsWindowInfo	bd_WinInfo;
	// struct ScreenModeRequester *	bd_ScreenModeRequester;
	// Object *						bd_MouseObject;
	// // ---
	// Object *						Gad_ScreenType;
	// Object *						Gad_ScreenMode;
	// Object *						Gad_Slider;
};

// --

#endif
