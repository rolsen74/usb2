
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef __AMIGAINPUT_AMIGAINTPUT_H__
#define __AMIGAINPUT_AMIGAINTPUT_H__

// --

typedef struct _AIN_DriverSettings
{
	S32		NumAxes;
	S32 *	Min;
	S32 *	Center;
	S32 *	Max;
	S32		NumHats;
	S32 *	HatMask;
	S32 *	HatEnable;
	S32		NumButtons;

} AIN_DriverSettings;

// --

extern const struct TagItem AI_Tags[];

// --

#endif
