#ifndef WIRELESS_RECEIVE_QUEUE_H
#define WIRELESS_RECEIVE_QUEUE_H

#include <devices/usb2.h>

#define WIRELESS_RX_REQUEST_COUNT 3

void Wireless_SubmitRX( struct USB2_EPResource *resource, U32 *pending );
struct USB2_IORequest *Wireless_TakeRX( struct USB2_EPResource *resource, U32 *pending );
void Wireless_StopRX( struct USB2_EPResource *resource, U32 *pending );

#endif
