
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef INC_PRINTER_H
#define INC_PRINTER_H

/***************************************************************************
** USB Printer Class Definitions
** Based on USB Printer Class Specification Revision 1.1
** https://www.usb.org/document-library/usb-printer-class-specification-rev-11
**
***************************************************************************/

// --
// SubClass codes

#define USBPRT_SUBCLASS_Printer				0x01

// --
// Protocol codes

#define USBPRT_PROTOCOL_UNIDIRECTIONAL		0x01
#define USBPRT_PROTOCOL_BIDIRECTIONAL		0x02
#define USBPRT_PROTOCOL_IEEE1284_4			0x03
#define USBPRT_PROTOCOL_VENDORSPECIFIC		0xFF

// --
// Printer request codes

#define USBPRT_REQC_GET_DEVICE_ID			0x00
#define USBPRT_REQC_GET_PORT_STATUS			0x01
#define USBPRT_REQC_SOFT_RESET				0x02

// --
// Printer status bits for GET_PORT_STATUS
// Status is 1 byte

// Bits 0-2 Reserved
#define USBPRT_STAT_NOERROR					0x08	// Bit 3: 1 = No error
#define USBPRT_STAT_SELECTED				0x10	// Bit 4: 1 = Selected
#define USBPRT_STAT_EMPTY					0x20	// Bit 5: 1 = Paper empty
// Bits 6-7 Reserved

// --

#endif

