
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
Audio_Stream_Interfaces( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	return ( Descriptor_List( out, data, length, offset,
		DESC_FIELDS( DESC_BYTE(baInterfaceNr) ), data[7] ) );
}

// --

static U32
Audio_Sample_Rates( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	if ( ! data[offset - 1] )
	{
		return ( Descriptor_Fields( out, data, length, offset,
			DESC_FIELDS( { "tLowerSamFreq", 3, DESC_DECIMAL }, { "tUpperSamFreq", 3, DESC_DECIMAL } ) ) );
	}

	return ( Descriptor_List( out, data, length, offset,
		DESC_FIELDS( { "tSamFreq", 3, DESC_DECIMAL } ), data[offset - 1] ) );
}

// --

static U32
Audio_Source_IDs( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	return ( Descriptor_List( out, data, length, offset,
		DESC_FIELDS( DESC_BYTE(baSourceID) ), data[offset - 1] ) );
}

// --

static U32
Audio_Feature_Controls( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset, U32 width )
{
	struct DescriptorField control;

	U32 count;

	if ( ( ! width ) || ( width > 4 ) || ( length <= offset ) )
	{
		return ( offset );
	}

	count = length - offset - 1;

	if ( count % width )
	{
		return ( offset );
	}

	control.Name = "bmaControls";
	control.Size = width;
	control.Format = DESC_HEX;

	offset = Descriptor_List( out, data, length, offset, & control, count / width );

	return ( Descriptor_Fields( out, data, length, offset, DESC_FIELDS( DESC_BYTE(iFeature) ) ) );
}

// --

static U32
UAC1_Feature_Controls( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	return ( Audio_Feature_Controls( out, data, length, offset, data[5] ) );
}

// --

static U32
UAC2_Feature_Controls( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	return ( Audio_Feature_Controls( out, data, length, offset, 4 ) );
}

// --

static U32
UAC1_Selector( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	offset = Audio_Source_IDs( out, data, length, offset );

	if ( offset != 5U + data[4] )
	{
		return ( offset );
	}

	return ( Descriptor_Fields( out, data, length, offset, DESC_FIELDS( DESC_BYTE(iSelector) ) ) );
}

// --

static U32
UAC2_Selector( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	offset = Audio_Source_IDs( out, data, length, offset );

	if ( offset != 5U + data[4] )
	{
		return ( offset );
	}

	return ( Descriptor_Fields( out, data, length, offset,
		DESC_FIELDS( DESC_BITS(bmControls, 1), DESC_BYTE(iSelector) ) ) );
}

// --

static U32
UAC2_Clock_Selector( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	offset = Descriptor_List( out, data, length, offset,
		DESC_FIELDS( DESC_BYTE(baCSourceID) ), data[4] );

	if ( offset != 5U + data[4] )
	{
		return ( offset );
	}

	return ( Descriptor_Fields( out, data, length, offset,
		DESC_FIELDS( DESC_BITS(bmControls, 1), DESC_BYTE(iClockSelector) ) ) );
}

// --

static U32
UAC1_Mixer( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	U32 controls;

	offset = Audio_Source_IDs( out, data, length, offset );

	if ( offset != 5U + data[4] )
	{
		return ( offset );
	}

	controls = offset + 4;

	offset = Descriptor_Fields( out, data, length, offset,
		DESC_FIELDS( DESC_BYTE(bNrChannels), DESC_BITS(wChannelConfig, 2), DESC_BYTE(iChannelNames) ) );

	if ( ( offset != controls ) || ( length <= offset ) )
	{
		return ( offset );
	}

	offset = Descriptor_List( out, data, length, offset,
		DESC_FIELDS( DESC_BITS(bmControls, 1) ), length - offset - 1 );

	return ( Descriptor_Fields( out, data, length, offset, DESC_FIELDS( DESC_BYTE(iMixer) ) ) );
}

// --

static U32
UAC2_Mixer( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	U32 controls;

	offset = Audio_Source_IDs( out, data, length, offset );

	if ( offset != 5U + data[4] )
	{
		return ( offset );
	}

	controls = offset + 6;

	offset = Descriptor_Fields( out, data, length, offset,
		DESC_FIELDS( DESC_BYTE(bNrChannels), DESC_BITS(bmChannelConfig, 4), DESC_BYTE(iChannelNames) ) );

	if ( ( offset != controls ) || ( length - offset < 2 ) )
	{
		return ( offset );
	}

	offset = Descriptor_List( out, data, length, offset,
		DESC_FIELDS( DESC_BITS(bmMixerControls, 1) ), length - offset - 2 );

	return ( Descriptor_Fields( out, data, length, offset,
		DESC_FIELDS( DESC_BITS(bmControls, 1), DESC_BYTE(iMixer) ) ) );
}

// --

static U32
MIDI_Jack_Pins( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	U32 count;
	U32 pin;

	count = data[5];

	for ( pin = 0; ( pin < count ) && ( length - offset >= 2 ); pin++ )
	{
		offset = Descriptor_Fields( out, data, length, offset,
			DESC_FIELDS( DESC_BYTE(baSourceID), DESC_BYTE(baSourcePin) ) );
	}

	if ( pin != count )
	{
		return ( offset );
	}

	return ( Descriptor_Fields( out, data, length, offset, DESC_FIELDS( DESC_BYTE(iJack) ) ) );
}

// --

static U32
MIDI_Endpoint_Jacks( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	return ( Descriptor_List( out, data, length, offset,
		DESC_FIELDS( DESC_BYTE(baAssocJackID) ), data[3] ) );
}

// --

static U32
MIDI_Endpoint_Blocks( struct DescriptorOutput *out, const U8 *data, U32 length, U32 offset )
{
	return ( Descriptor_List( out, data, length, offset,
		DESC_FIELDS( DESC_BYTE(baAssoGrpTrmBlkID) ), data[3] ) );
}

// --

static const struct ClassDescriptor UAC1Control[] =
{
	[1] = { "UAC1 AudioControl header",
		DESC_FIELDS( DESC_VERSION(bcdADC), DESC_WORD(wTotalLength), DESC_BYTE(bInCollection) ), Audio_Stream_Interfaces },
	[2] = { "UAC1 input terminal",
		DESC_FIELDS( DESC_BYTE(bTerminalID), DESC_BITS(wTerminalType, 2), DESC_BYTE(bAssocTerminal),
			DESC_BYTE(bNrChannels), DESC_BITS(wChannelConfig, 2), DESC_BYTE(iChannelNames), DESC_BYTE(iTerminal) ), NULL },
	[3] = { "UAC1 output terminal",
		DESC_FIELDS( DESC_BYTE(bTerminalID), DESC_BITS(wTerminalType, 2), DESC_BYTE(bAssocTerminal),
			DESC_BYTE(bSourceID), DESC_BYTE(iTerminal) ), NULL },
	[4] = { "UAC1 mixer unit", DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BYTE(bNrInPins) ), UAC1_Mixer },
	[5] = { "UAC1 selector unit", DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BYTE(bNrInPins) ), UAC1_Selector },
	[6] = { "UAC1 feature unit", DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BYTE(bSourceID), DESC_BYTE(bControlSize) ),
		UAC1_Feature_Controls },
	[7] = { "UAC1 processing unit",
		DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BITS(wProcessType, 2), DESC_BYTE(bNrInPins) ), Audio_Source_IDs },
	[8] = { "UAC1 extension unit",
		DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BITS(wExtensionCode, 2), DESC_BYTE(bNrInPins) ), Audio_Source_IDs }
};

static const struct ClassDescriptor UAC2Control[] =
{
	[1] = { "UAC2 AudioControl header",
		DESC_FIELDS( DESC_VERSION(bcdADC), DESC_BYTE(bCategory), DESC_WORD(wTotalLength), DESC_BITS(bmControls, 1) ), NULL },
	[2] = { "UAC2 input terminal",
		DESC_FIELDS( DESC_BYTE(bTerminalID), DESC_BITS(wTerminalType, 2), DESC_BYTE(bAssocTerminal), DESC_BYTE(bCSourceID),
			DESC_BYTE(bNrChannels), DESC_BITS(bmChannelConfig, 4), DESC_BYTE(iChannelNames),
			DESC_BITS(bmControls, 2), DESC_BYTE(iTerminal) ), NULL },
	[3] = { "UAC2 output terminal",
		DESC_FIELDS( DESC_BYTE(bTerminalID), DESC_BITS(wTerminalType, 2), DESC_BYTE(bAssocTerminal),
			DESC_BYTE(bSourceID), DESC_BYTE(bCSourceID), DESC_BITS(bmControls, 2), DESC_BYTE(iTerminal) ), NULL },
	[4] = { "UAC2 mixer unit", DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BYTE(bNrInPins) ), UAC2_Mixer },
	[5] = { "UAC2 selector unit", DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BYTE(bNrInPins) ), UAC2_Selector },
	[6] = { "UAC2 feature unit", DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BYTE(bSourceID) ), UAC2_Feature_Controls },
	[7] = { "UAC2 effect unit",
		DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BITS(wEffectType, 2), DESC_BYTE(bSourceID) ), NULL },
	[8] = { "UAC2 processing unit",
		DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BITS(wProcessType, 2), DESC_BYTE(bNrInPins) ), Audio_Source_IDs },
	[9] = { "UAC2 extension unit",
		DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BITS(wExtensionCode, 2), DESC_BYTE(bNrInPins) ), Audio_Source_IDs },
	[10] = { "UAC2 clock source", DESC_FIELDS( DESC_BYTE(bClockID), DESC_BITS(bmAttributes, 1),
			DESC_BITS(bmControls, 1), DESC_BYTE(bAssocTerminal), DESC_BYTE(iClockSource) ), NULL },
	[11] = { "UAC2 clock selector", DESC_FIELDS( DESC_BYTE(bClockID), DESC_BYTE(bNrInPins) ), UAC2_Clock_Selector },
	[12] = { "UAC2 clock multiplier", DESC_FIELDS( DESC_BYTE(bClockID), DESC_BYTE(bCSourceID),
			DESC_BITS(bmControls, 1), DESC_BYTE(iClockMultiplier) ), NULL },
	[13] = { "UAC2 sample rate converter",
		DESC_FIELDS( DESC_BYTE(bUnitID), DESC_BYTE(bSourceID), DESC_BYTE(bCSourceInID),
			DESC_BYTE(bCSourceOutID), DESC_BYTE(iSRC) ), NULL }
};

static const struct ClassDescriptor UAC1Streaming[] =
{
	[1] = { "UAC1 AudioStreaming general",
		DESC_FIELDS( DESC_BYTE(bTerminalLink), DESC_BYTE(bDelay), DESC_BITS(wFormatTag, 2) ), NULL },
	[2] = { "UAC1 audio format", DESC_FIELDS( DESC_BYTE(bFormatType) ), NULL },
	[3] = { "UAC1 format-specific", DESC_FIELDS( DESC_BITS(wFormatTag, 2) ), NULL }
};

static const struct ClassDescriptor UAC2Streaming[] =
{
	[1] = { "UAC2 AudioStreaming general",
		DESC_FIELDS( DESC_BYTE(bTerminalLink), DESC_BITS(bmControls, 1), DESC_BYTE(bFormatType), DESC_BITS(bmFormats, 4),
			DESC_BYTE(bNrChannels), DESC_BITS(bmChannelConfig, 4), DESC_BYTE(iChannelNames) ), NULL },
	[2] = { "UAC2 audio format", DESC_FIELDS( DESC_BYTE(bFormatType) ), NULL },
	[3] = { "UAC2 audio encoder", DESC_FIELDS( DESC_BYTE(bEncoderID), DESC_BYTE(bEncoder),
			DESC_BITS(bReserved, 3), DESC_BITS(bmControls, 4), DESC_BYTE(iParam1), DESC_BYTE(iParam2),
			DESC_BYTE(iParam3), DESC_BYTE(iParam4), DESC_BYTE(iParam5), DESC_BYTE(iParam6),
			DESC_BYTE(iParam7), DESC_BYTE(iParam8), DESC_BYTE(iEncoder) ), NULL },
	[4] = { "UAC2 audio decoder", DESC_FIELDS( DESC_BYTE(bDecoderID), DESC_BYTE(bDecoder) ), NULL }
};

static const struct ClassDescriptor UAC1Format =
{
	"UAC1 audio format",
	DESC_FIELDS( DESC_BYTE(bFormatType), DESC_BYTE(bNrChannels), DESC_BYTE(bSubframeSize),
		DESC_BYTE(bBitResolution), DESC_BYTE(bSamFreqType) ),
	Audio_Sample_Rates
};

static const struct ClassDescriptor UAC1FormatII =
{
	"UAC1 Type II audio format",
	DESC_FIELDS( DESC_BYTE(bFormatType), DESC_WORD(wMaxBitRate), DESC_WORD(wSamplesPerFrame), DESC_BYTE(bSamFreqType) ),
	Audio_Sample_Rates
};

static const struct ClassDescriptor UAC2Format =
{
	"UAC2 audio format",
	DESC_FIELDS( DESC_BYTE(bFormatType), DESC_BYTE(bSubslotSize), DESC_BYTE(bBitResolution) ),
	NULL
};

static const struct ClassDescriptor UAC2FormatII =
{
	"UAC2 Type II audio format",
	DESC_FIELDS( DESC_BYTE(bFormatType), DESC_WORD(wMaxBitRate), DESC_WORD(wSlotsPerFrame) ),
	NULL
};

static const struct ClassDescriptor UAC1Endpoint =
{
	"UAC1 AudioStreaming data endpoint",
	DESC_FIELDS( DESC_BITS(bmAttributes, 1), DESC_BYTE(bLockDelayUnits), DESC_WORD(wLockDelay) ),
	NULL
};

static const struct ClassDescriptor UAC2Endpoint =
{
	"UAC2 AudioStreaming data endpoint",
	DESC_FIELDS( DESC_BITS(bmAttributes, 1), DESC_BITS(bmControls, 1),
		DESC_BYTE(bLockDelayUnits), DESC_WORD(wLockDelay) ),
	NULL
};

static const struct ClassDescriptor MIDIStreaming[] =
{
	[1] = { "MIDIStreaming header", DESC_FIELDS( DESC_VERSION(bcdMSC), DESC_WORD(wTotalLength) ), NULL },
	[2] = { "MIDI IN jack", DESC_FIELDS( DESC_BYTE(bJackType), DESC_BYTE(bJackID), DESC_BYTE(iJack) ), NULL },
	[3] = { "MIDI OUT jack", DESC_FIELDS( DESC_BYTE(bJackType), DESC_BYTE(bJackID), DESC_BYTE(bNrInputPins) ),
		MIDI_Jack_Pins },
	[4] = { "MIDI element", DESC_FIELDS( DESC_BYTE(bElementID), DESC_BYTE(bNrInputPins) ), NULL }
};

static const struct ClassDescriptor MIDIEndpoints[] =
{
	[1] = { "MIDI 1.0 data endpoint", DESC_FIELDS( DESC_BYTE(bNumEmbMIDIJack) ), MIDI_Endpoint_Jacks },
	[2] = { "MIDI 2.0 data endpoint", DESC_FIELDS( DESC_BYTE(bNumGrpTrmBlock) ), MIDI_Endpoint_Blocks }
};

// --

const struct ClassDescriptor *
Audio_Descriptor( const U8 *interface, const U8 *data, U32 length )
{
	U32 subclass;
	U32 protocol;
	U32 subtype;

	if ( length < 3 )
	{
		return ( NULL );
	}

	subclass = interface[6];
	protocol = interface[7];
	subtype = data[2];

	if ( subclass == 3 )
	{
		if ( data[1] == DESC_CLASS_INTERFACE )
		{
			return ( DESC_LOOKUP( MIDIStreaming, subtype ) );
		}

		if ( data[1] == DESC_CLASS_ENDPOINT )
		{
			return ( DESC_LOOKUP( MIDIEndpoints, subtype ) );
		}

		return ( NULL );
	}

	if ( ( protocol != 0 ) && ( protocol != 0x20 ) )
	{
		return ( NULL );
	}

	if ( ( data[1] == DESC_CLASS_ENDPOINT ) && ( subclass == 2 ) && ( subtype == 1 ) )
	{
		return ( protocol ? & UAC2Endpoint : & UAC1Endpoint );
	}

	if ( data[1] != DESC_CLASS_INTERFACE )
	{
		return ( NULL );
	}

	if ( subclass == 1 )
	{
		return ( protocol ? DESC_LOOKUP( UAC2Control, subtype ) : DESC_LOOKUP( UAC1Control, subtype ) );
	}

	if ( subclass == 2 )
	{
		if ( ( subtype == 2 ) && ( length >= 4 ) )
		{
			switch ( data[3] )
			{
				case 1:
				case 3:
					return ( protocol ? & UAC2Format : & UAC1Format );
				case 2:
					return ( protocol ? & UAC2FormatII : & UAC1FormatII );
			}
		}

		return ( protocol ? DESC_LOOKUP( UAC2Streaming, subtype ) : DESC_LOOKUP( UAC1Streaming, subtype ) );
	}

	return ( NULL );
}
