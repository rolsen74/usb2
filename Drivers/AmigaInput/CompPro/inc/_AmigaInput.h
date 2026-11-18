
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

#include "CompPro.h"

#include <proto/amigainput.h>
#include <proto/amigainput_driver.h>

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

S32	_AI_CleanUp(			struct AINSubIFace *Self, AIN_DeviceID ID );
S32	_AI_HandleSettings(		struct AINSubIFace *Self, AIN_DeviceID ID, AIN_DriverSettings *settings, BOOL load, BOOL use );
S32	_AI_InitDriver(			struct AINSubIFace *Self );
U32	_AI_Obtain(			struct AINSubIFace *Self );
S32	_AI_Read_KBD(			struct AINSubIFace *Self, AIN_DeviceID ID, STR puffer );
void	_AI_Read(			struct AINSubIFace *Self, AIN_DeviceID ID );
U32	_AI_Release(			struct AINSubIFace *Self );
void	_AI_SetCalibrateMode(	struct AINSubIFace *Self, AIN_DeviceID ID, BOOL Mode );
U32	_AI_SetEventMode(		struct AINSubIFace *Self, AIN_DeviceID ID, U32 MOD );
U32	_AI_TestUnit(			struct AINSubIFace *Self, AIN_DeviceID ID );

// --

#endif
