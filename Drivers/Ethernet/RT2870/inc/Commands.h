
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef RT2870_COMMANDS_H
#define RT2870_COMMANDS_H

#define S2_SETOPTIONS 0xc012
#define S2_SETKEY 0xc013
#define S2ENC_CCMP 3
#define S2ENC_TKIP 2
#define S2_READMGMT 0xc015
#define S2_WRITEMGMT 0xc016
#define S2INFO_BSSID ( TAG_USER + 1 )
#define S2INFO_AssocID ( TAG_USER + 3 )
#define S2INFO_Encryption ( TAG_USER + 4 )
#define S2INFO_PortType ( TAG_USER + 5 )
#define S2INFO_Channel ( TAG_USER + 7 )
#define S2INFO_OperState ( TAG_USER + 20 )
#define S2PORT_MANAGED 7
#define RT2870_MAX_CHANNEL 13
#define RT2870_EAPOL_M2 0x010a
#define RT2870_EAPOL_M4 0x030a

void Cmd_Invalid( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_Read( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Write( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Flush( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_DeviceQuery( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_GetStationAddress( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_ConfigInterface( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_OnEvent( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Online( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_Offline( struct EthernetUnit *unit, struct IOSana2Req *ioreq );

void Cmd_Sana2Hook( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_SetKey( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
void Cmd_SetOptions( struct EthernetUnit *unit, struct IOSana2Req *ioreq );
S32 Cmd_ReadMgmt( struct EthernetUnit *unit, struct IOSana2Req *ioreq );

#endif
