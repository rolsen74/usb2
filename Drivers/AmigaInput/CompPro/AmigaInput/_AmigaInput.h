
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
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
