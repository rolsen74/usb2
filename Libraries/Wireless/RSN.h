
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_WIRELESS_RSN_H
#define USB2_WIRELESS_RSN_H

#include <stdint.h>

int32_t WPA_Supports_TKIP( const uint8_t *data, uint32_t length );
int32_t RSN_Supports_WPA2( const uint8_t *data, uint32_t length );
int32_t RSN_Supports_SAE( const uint8_t *data, uint32_t length );
int32_t RSNX_Supports_H2E( const uint8_t *data, uint32_t length );

#endif
