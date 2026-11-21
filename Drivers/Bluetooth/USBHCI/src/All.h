#ifndef USB2_BLUETOOTH_USB_INTERNAL_H
#define USB2_BLUETOOTH_USB_INTERNAL_H

#include <devices/usb2.h>
#include <dos/dos.h>
#include <interfaces/usb2.h>
#include <interfaces/bluetooth2.h>

#define HCI_USB_TIMEOUT 1500000

enum BTUSBEventRead
{
	BTUSB_EVENT_IDLE,
	BTUSB_EVENT_ON_DEMAND,
	BTUSB_EVENT_PREFETCH,
};

struct BTUSB
{
	struct USB2IFace *USB2;
	struct USB2_Register *Registration;
	struct USB2_EPResource *Events;
	struct USB2_EPResource *ACLIn;
	struct USB2_EPResource *ACLOut;
	U32 InterfaceNumber;
	enum BTUSBEventRead EventPending;
	S32 PrefetchEvents;
};

U8 BTUSB_FindEventEndpoint( struct USB2IFace *usb2, struct USB2_Interface *ifc );
S32 BTUSB_Open( struct BTUSB *usb, struct USB2_DriverMessage *message, U8 endpoint );
S32 BTUSB_InitializeRealtek( struct BTUSB *usb, U16 vendor, U16 product );
S32 BTUSB_OpenACL( struct BTUSB *usb );
S32 BTUSB_ReadACLChunk( PTR context, const U8 **data, U32 *length );
S32 BTUSB_WriteACL( PTR context, const U8 *data, U32 length );
void BTUSB_Close( struct BTUSB *usb );
U32 BTUSB_Entry( struct USB2DriverIFace *self, struct USB2_DriverMessage *message );
S32 BTUSB_StartCommand( PTR context, const U8 *command, U32 length );
S32 BTUSB_ReadEventChunk( PTR context, const U8 **data, U32 *length );
void BTUSB_QueueEvent( struct BTUSB *usb, enum BTUSBEventRead kind );
void BTUSB_CancelEvent( struct BTUSB *handle );

#endif
