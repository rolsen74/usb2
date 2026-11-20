
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#ifndef USB2_LEGACY_WPA_H
#define USB2_LEGACY_WPA_H

#include "All.h"

// --

extern const U8 AssociationWPA[USB2WPA_ASSOC_IE_SIZE];

struct USB2WPASession * APICALL WPA_Create( struct USB2WPAIFace *self,
	const struct USB2WPANetwork *network, const char *password, U32 length, S32 *error );
void APICALL WPA_Destroy( struct USB2WPAIFace *self, struct USB2WPASession *session );
S32 APICALL WPA_Process( struct USB2WPAIFace *self, struct USB2WPASession *session,
	const U8 *frame, U32 length, struct USB2WPAReply *reply );

#endif
