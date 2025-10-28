
/*
** Universal serial bus attempt by Rene W. Olsen
**
** Copyright (c) 2012-2025 by Rene W. Olsen < renewolsen @ gmail . com >
** All rights reserved.
**
*/

#ifndef PROTO_USB2_H
#define PROTO_USB2_H

/****************************************************************************/

#ifndef DEVICES_USB2_H
#include <devices/usb2.h>
#endif

/****************************************************************************/

#ifdef __amigaos4__

  #include <interfaces/usb2.h>

  #ifdef __USE_INLINE__
  #include <inline4/usb2.h>
  #endif

  #ifndef CLIB_USB2_PROTOS_H
  #define CLIB_USB2_PROTOS_H 1
  #endif

  #ifndef __NOLIBBASE__
  extern struct Device *USB2Base;
  #endif

  #ifndef __NOGLOBALIFACE__
  extern struct USB2_IFace *IUSB2;
  #endif

#else // Other platforms

  #error USB2 is only supported on AmigaOS4 .. at the moment

#endif

/****************************************************************************/

#endif
