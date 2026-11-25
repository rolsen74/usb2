
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef PEGASUS_WIRED_CHIP_H
#define PEGASUS_WIRED_CHIP_H

#define WIRED_CHIP_FIELDS \
	U8 unit_PegasusPHY; \
	U8 unit_PegasusMode; \
	U8 unit_PegasusMACState; \
	U8 unit_PegasusStartFailures; \
	U8 unit_PegasusStopFailures; \
	U8 unit_PegasusRXPrefix; \
	U8 unit_PegasusRXStarted; \
	U8 unit_PegasusRXLogged; \
	U8 unit_PegasusRXRejected; \
	U8 unit_PegasusRXErrors; \
	U8 unit_PegasusTXStarted; \
	U8 unit_PegasusTXLogged; \
	U8 unit_PegasusTXErrors; \
	U8 unit_PegasusStatusPending; \
	U8 unit_PegasusRXDeliveryLogged;

#define WIRED_NAME "Pegasus"
#define WIRED_ADAPTER USB2ETH_ADAPTER_UNKNOWN
#define WIRED_RX_REQUEST_COUNT 1
#define WIRED_TX_REQUEST_COUNT 1
#define WIRED_RX_BUFFER_SIZE 1536
#define WIRED_TX_HEADER_SIZE 2
#define WIRED_TX_BUFFER_SIZE ( ETHERNET_MTU + WIRED_TX_HEADER_SIZE + 1 )
#define WIRED_TX_TIMEOUT 5000000UL

#define WIRED_RESET_RX(unit) ((void) (unit))
#define WIRED_STOP_TRAFFIC(unit) Pegasus_StopTraffic( unit )
#define WIRED_TRACE_BULK(unit, request, receive) Pegasus_TraceBulk( unit, request, receive )
#define WIRED_TRACE_RX_START(unit, request) Pegasus_TraceRXStart( unit, request )
#define WIRED_TRACE_RX_DELIVERY(unit, type, result) Pegasus_TraceRXDelivery( unit, type, result )

struct devUnit;
struct USB2_IORequest;

S32 Pegasus_StopTraffic( struct devUnit *unit );
void Pegasus_TraceBulk( struct devUnit *unit, const struct USB2_IORequest *request, S32 receive );
void Pegasus_TraceRXStart( struct devUnit *unit, const struct USB2_IORequest *request );
void Pegasus_TraceRXDelivery( struct devUnit *unit, U32 type, U32 result );

#endif
