#ifndef USB2_BLUETOOTH_HID_EVENTS_H
#define USB2_BLUETOOTH_HID_EVENTS_H

#include "Bluetooth.h"

struct BTHIDEvents
{
	void *Context;
	int32_t (*Emit)( void *context, uint8_t kind, uint16_t code, uint16_t qualifier, int16_t x, int16_t y );
	uint16_t Qualifier;
	uint8_t Keys[6];
	int16_t Codes[6];
	uint8_t Modifiers;
	uint8_t Buttons;
	uint8_t NumLock;
	uint8_t Failed;
};

int32_t ApplyHIDReport( struct BTHIDEvents *events, const struct BTHIDReport *report );

#endif
