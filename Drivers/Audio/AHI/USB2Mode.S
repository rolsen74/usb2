/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifdef BLUETOOTH_AHI
#define AHI_DRIVER "bluetooth2"
#define AHI_MODE_NAME "Bluetooth2: A2DP stereo playback"
#else
#define AHI_DRIVER "usb2"
#define AHI_MODE_NAME "USB2: 16-bit stereo playback"
#endif

	.section .rodata
	.ascii "FORM"
	.long form_end - form
form:
	.ascii "AHIM"
	.ascii "AUDN"
	.long driver_end - driver
driver:
	.asciz AHI_DRIVER
driver_end:
	.balign 2
	.ascii "AUDM"
	.long mode_end - mode
mode:
	.long 0x80000064, USB2_AHI_MODE_ID	/* AHIDB_AudioID */
	.long 0x80000067, 1			/* AHIDB_Volume */
	.long 0x80000068, 1			/* AHIDB_Panning */
	.long 0x80000069, 1			/* AHIDB_Stereo */
	.long 0x8000806d, mode_name - mode	/* AHIDB_Name */
	.long 0, 0				/* TAG_END */
mode_name:
	.asciz AHI_MODE_NAME
mode_end:
	.balign 2
form_end:
