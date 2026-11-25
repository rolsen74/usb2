
/*
**      -+- Universal serial bus -+-
** Copyright (c) 2012-2026 by Rene W. Olsen
**
** SPDX-License-Identifier: GPL-3.0-or-later
*/

// --

#include "inc/All.h"
#include "inc/DescriptorFields.h"

// --

static U32
Video_Stream_Interfaces( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	return ( Descriptor_List( out, data, length, offset,
		DESC_FIELDS( DESC_BYTE(baInterfaceNr) ), data[11] ) );
}

// --

static U32
Camera_Controls( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	return ( Descriptor_List( out, data, length, offset,
		DESC_FIELDS( DESC_BITS(bmControls, 1) ), data[14] ) );
}

// --

static U32
Selector_Inputs( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	offset = Descriptor_List( out, data, length, offset, DESC_FIELDS( DESC_BYTE(baSourceID) ), data[4] );

	if ( offset != 5U + data[4] )
	{
		return ( offset );
	}

	return ( Descriptor_Fields( out, data, length, offset, DESC_FIELDS( DESC_BYTE(iSelector) ) ) );
}

// --

static U32
Processing_Controls( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	offset = Descriptor_List( out, data, length, offset,
		DESC_FIELDS( DESC_BITS(bmControls, 1) ), data[7] );

	if ( offset != 8U + data[7] )
	{
		return ( offset );
	}

	offset = Descriptor_Fields( out, data, length, offset, DESC_FIELDS( DESC_BYTE(iProcessing) ) );

	// UVC 1.0 ends at iProcessing.
	if ( length > offset )
	{
		offset = Descriptor_Fields( out, data, length, offset, DESC_FIELDS( DESC_BITS(bmVideoStandards, 1) ) );
	}

	return ( offset );
}

// --

static U32
Extension_Controls( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	U32 count;
	U32 first_control;

	offset = Descriptor_List( out, data, length, offset, DESC_FIELDS( DESC_BYTE(baSourceID) ), data[21] );

	if ( ( offset != 22U + data[21] ) || ( offset == length ) )
	{
		return ( offset );
	}

	count = data[offset];

	first_control = Descriptor_Fields( out, data, length, offset, DESC_FIELDS( DESC_BYTE(bControlSize) ) );

	offset = Descriptor_List( out, data, length, first_control, DESC_FIELDS( DESC_BITS(bmControls, 1) ), count );

	if ( offset != first_control + count )
	{
		return ( offset );
	}

	return ( Descriptor_Fields( out, data, length, offset, DESC_FIELDS( DESC_BYTE(iExtension) ) ) );
}

// --

static U32
Stream_Controls( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	return ( Descriptor_List( out, data, length, offset,
		DESC_FIELDS( DESC_BITS(bmaControls, 1) ), (U32)data[3] * data[offset - 1] ) );
}

// --

static U32
Encoding_Controls( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	U32 size;
	U32 runtime;

	size = data[6];
	runtime = offset + size;

	offset = Descriptor_List( out, data, length, offset, DESC_FIELDS( DESC_BITS(bmControls, 1) ), size );

	if ( offset != runtime )
	{
		return ( offset );
	}

	return ( Descriptor_List( out, data, length, offset, DESC_FIELDS( DESC_BITS(bmControlsRuntime, 1) ), size ) );
}

// --

static U32
Frame_Intervals( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	U32 count;

	// Frame-based payloads put the interval count before dwBytesPerLine.
	count = ( data[2] == 0x11 ) ? data[21] : data[25];

	if ( ! count )
	{
		return ( Descriptor_Fields( out, data, length, offset,
			DESC_FIELDS( DESC_LONG(dwMinFrameInterval), DESC_LONG(dwMaxFrameInterval),
				DESC_LONG(dwFrameIntervalStep) ) ) );
	}

	return ( Descriptor_List( out, data, length, offset,
		DESC_FIELDS( DESC_LONG(dwFrameInterval) ), count ) );
}

// --

static U32
Still_Image_Patterns( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	U32 count;
	U32 index;

	count = data[4];

	for ( index = 0; ( index < count ) && ( length - offset >= 4 ); index++ )
	{
		offset = Descriptor_Fields( out, data, length, offset,
			DESC_FIELDS( DESC_WORD(wWidth), DESC_WORD(wHeight) ) );
	}

	if ( ( index != count ) || ( offset == length ) )
	{
		return ( offset );
	}

	count = data[offset];

	offset = Descriptor_Fields( out, data, length, offset, DESC_FIELDS( DESC_BYTE(bNumCompressionPatterns) ) );

	return ( Descriptor_List( out, data, length, offset, DESC_FIELDS( DESC_BYTE(bCompression) ), count ) );
}

// --

static const struct ClassDescriptor VideoControl[] =
{
	[1] = { "UVC VideoControl header",
		DESC_FIELDS( DESC_VERSION(bcdUVC), DESC_WORD(wTotalLength), DESC_LONG(dwClockFrequency),
			DESC_BYTE(bInCollection) ), Video_Stream_Interfaces },
	[2] = { "UVC input terminal",
		DESC_FIELDS( DESC_BYTE(bTerminalID), DESC_BITS(wTerminalType, 2), DESC_BYTE(bAssocTerminal),
			DESC_BYTE(iTerminal) ), NULL },
	[3] = { "UVC output terminal",
		DESC_FIELDS( DESC_BYTE(bTerminalID), DESC_BITS(wTerminalType, 2), DESC_BYTE(bAssocTerminal),
			DESC_BYTE(bSourceID), DESC_BYTE(iTerminal) ), NULL },
	[4] = { "UVC selector unit", DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BYTE(bNrInPins) ), Selector_Inputs },
	[5] = { "UVC processing unit",
		DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BYTE(bSourceID), DESC_WORD(wMaxMultiplier), DESC_BYTE(bControlSize) ),
		Processing_Controls },
	[6] = { "UVC extension unit",
		DESC_FIELDS( DESC_BYTE(bUnitID), DESC_UUID(guidExtensionCode), DESC_BYTE(bNumControls), DESC_BYTE(bNrInPins) ),
		Extension_Controls },
	[7] = { "UVC encoding unit",
		DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BYTE(bSourceID), DESC_BYTE(iEncoding), DESC_BYTE(bControlSize) ),
		Encoding_Controls }
};

static const struct ClassDescriptor CameraTerminal =
{
	"UVC camera input terminal",
	DESC_FIELDS( DESC_BYTE(bTerminalID), DESC_BITS(wTerminalType, 2), DESC_BYTE(bAssocTerminal), DESC_BYTE(iTerminal),
		DESC_WORD(wObjectiveFocalLengthMin), DESC_WORD(wObjectiveFocalLengthMax),
		DESC_WORD(wOcularFocalLength), DESC_BYTE(bControlSize) ),
	Camera_Controls
};

static const struct ClassDescriptor VideoInterrupt =
{
	"UVC VideoControl interrupt endpoint", DESC_FIELDS( DESC_WORD(wMaxTransferSize) ), NULL
};

#define UVC_FORMAT_FIELDS DESC_BYTE(bFormatIndex), DESC_BYTE(bNumFrameDescriptors)
#define UVC_FORMAT_DEFAULTS DESC_BYTE(bDefaultFrameIndex), DESC_BYTE(bAspectRatioX), DESC_BYTE(bAspectRatioY), \
	DESC_BITS(bmInterlaceFlags, 1), DESC_BITS(bCopyProtect, 1)
#define UVC_FRAME_FIELDS DESC_BYTE(bFrameIndex), DESC_BITS(bmCapabilities, 1), DESC_WORD(wWidth), DESC_WORD(wHeight), \
	DESC_LONG(dwMinBitRate), DESC_LONG(dwMaxBitRate)
#define UVC_FRAME_INTERVALS DESC_LONG(dwMaxVideoFrameBufferSize), DESC_LONG(dwDefaultFrameInterval), \
	DESC_BYTE(bFrameIntervalType)

static const struct ClassDescriptor VideoStreaming[] =
{
	[0x01] = { "UVC VideoStreaming input header",
		DESC_FIELDS( DESC_BYTE(bNumFormats), DESC_WORD(wTotalLength), DESC_BITS(bEndpointAddress, 1),
			DESC_BITS(bmInfo, 1), DESC_BYTE(bTerminalLink), DESC_BYTE(bStillCaptureMethod),
			DESC_BYTE(bTriggerSupport), DESC_BYTE(bTriggerUsage), DESC_BYTE(bControlSize) ), Stream_Controls },
	[0x02] = { "UVC VideoStreaming output header",
		DESC_FIELDS( DESC_BYTE(bNumFormats), DESC_WORD(wTotalLength), DESC_BITS(bEndpointAddress, 1),
			DESC_BYTE(bTerminalLink), DESC_BYTE(bControlSize) ), Stream_Controls },
	[0x03] = { "UVC still image frame", DESC_FIELDS( DESC_BITS(bEndpointAddress, 1), DESC_BYTE(bNumImageSizePatterns) ),
		Still_Image_Patterns },
	[0x04] = { "UVC uncompressed format",
		DESC_FIELDS( UVC_FORMAT_FIELDS, DESC_UUID(guidFormat), DESC_BYTE(bBitsPerPixel), UVC_FORMAT_DEFAULTS ), NULL },
	[0x05] = { "UVC uncompressed frame", DESC_FIELDS( UVC_FRAME_FIELDS, UVC_FRAME_INTERVALS ), Frame_Intervals },
	[0x06] = { "UVC MJPEG format",
		DESC_FIELDS( UVC_FORMAT_FIELDS, DESC_BITS(bmFlags, 1), UVC_FORMAT_DEFAULTS ), NULL },
	[0x07] = { "UVC MJPEG frame", DESC_FIELDS( UVC_FRAME_FIELDS, UVC_FRAME_INTERVALS ), Frame_Intervals },
	[0x0d] = { "UVC color matching",
		DESC_FIELDS( DESC_BYTE(bColorPrimaries), DESC_BYTE(bTransferCharacteristics), DESC_BYTE(bMatrixCoefficients) ),
		NULL },
	[0x10] = { "UVC frame-based format",
		DESC_FIELDS( UVC_FORMAT_FIELDS, DESC_UUID(guidFormat), DESC_BYTE(bBitsPerPixel), UVC_FORMAT_DEFAULTS,
			DESC_BYTE(bVariableSize) ), NULL },
	[0x11] = { "UVC frame-based frame",
		DESC_FIELDS( UVC_FRAME_FIELDS, DESC_LONG(dwDefaultFrameInterval), DESC_BYTE(bFrameIntervalType),
			DESC_LONG(dwBytesPerLine) ), Frame_Intervals },
	[0x12] = { "UVC stream-based format",
		DESC_FIELDS( DESC_BYTE(bFormatIndex), DESC_UUID(guidFormat), DESC_LONG(dwPacketLength) ), NULL }
};

// --

const struct ClassDescriptor *
Video_Descriptor( const U8 *interface, const U8 *data, U32 length )
{
	if ( length < 3 )
	{
		return ( NULL );
	}

	if ( ( data[1] == DESC_CLASS_ENDPOINT ) && ( interface[6] == 1 ) && ( data[2] == 3 ) )
	{
		return ( & VideoInterrupt );
	}

	if ( data[1] != DESC_CLASS_INTERFACE )
	{
		return ( NULL );
	}

	if ( interface[6] == 1 )
	{
		if ( ( data[2] == 2 ) && ( length >= 6 ) && ( data[4] == 0x01 ) && ( data[5] == 0x02 ) )
		{
			return ( & CameraTerminal );
		}

		return ( DESC_LOOKUP( VideoControl, data[2] ) );
	}

	if ( interface[6] == 2 )
	{
		return ( DESC_LOOKUP( VideoStreaming, data[2] ) );
	}

	return ( NULL );
}
