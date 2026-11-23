
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef WIFI_LIBRARY_ALL_H
#define WIFI_LIBRARY_ALL_H

#include <dos/dos.h>
#include <dos/var.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <usb2wifi.h>

// --

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef int32_t S32;

extern struct SignalSemaphore WiFiLock;
extern const APTR WiFiVectors[];
extern struct MsgPort *WiFiWorkerPort;

struct WiFiScan;

S32 WiFi_WorkersActive( void );
S32 WiFi_Run( const char *device, U32 unit, S32 setup, ULONG expected, ULONG registration );
ULONG WiFi_AdapterProtocol( U32 adapter );
S32 WiFi_LiveRegistration( U32 unit, ULONG *registration );
ULONG WiFi_ClaimedRegistration( U32 adapter, U32 unit );
S32 WiFi_UnitClaimed( U32 adapter, U32 unit, ULONG registration );
void WiFi_FindPassword( struct USB2WiFiProfile *network );
void WiFi_RememberScan( U32 adapter, U32 unit, const struct WiFiScan *scan );
void WiFi_RestoreScan( U32 adapter, U32 unit, struct WiFiScan *scan );

S32 WiFi_Window( struct USB2WiFiProfile *profile, U32 adapter, U32 unit, S32 setup, S32 retry, U32 *action );
void WiFi_CloseIntuition( void );
S32 WiFi_ObtainIntuition( void );
S32 APICALL WiFi_EditProfiles( struct USB2WiFiIFace *self );
S32 APICALL WiFi_Setup( struct USB2WiFiIFace *self, struct USB2WiFiProfile *profile, U32 adapter, U32 unit, U32 *action );
S32 APICALL WiFi_SetupRetry( struct USB2WiFiIFace *self, struct USB2WiFiProfile *profile, U32 adapter, U32 unit, U32 *action );
S32 APICALL WiFi_StartPrompt( struct USB2WiFiIFace *self, struct USB2WiFiPrompt *prompt );
S32 APICALL WiFi_StartPromptForUnit( struct USB2WiFiIFace *self, struct USB2WiFiPrompt *prompt, U32 adapter, U32 unit );
S32 APICALL WiFi_StartAuto( struct USB2WiFiIFace *self, U32 adapter, U32 unit );
void APICALL WiFi_CancelPrompt( struct USB2WiFiIFace *self, struct USB2WiFiPrompt *prompt );

U32 APICALL WiFi_InterfaceObtain( struct Interface *self );
U32 APICALL WiFi_InterfaceRelease( struct Interface *self );

#endif
