#ifndef USB2_AUDIO_TEST_WAV_H
#define USB2_AUDIO_TEST_WAV_H

#define AUDIO_WAV_MAX_HEADER_SIZE 68

struct AudioWav
{
	U8 *Samples;
	U32 Frames;
	U32 Rate;
	U32 Channels;
};

struct AudioWavResampler
{
	U64 Phase;
	S32 Previous[2];
	S32 Ready;
};

S32 Audio_Wav_Parse( U8 *data, U32 length, struct AudioWav *wav );
void Audio_WriteLE16( U8 *data, U32 value );
U32 Audio_Wav_HeaderSize( U32 channels );
U32 Audio_Wav_Header( U8 *header, U32 length, U32 rate, U32 channels );
void Audio_Wav_Convert( const struct AudioWav *wav, U8 *output, U32 frames, U32 rate, U32 channels );

U32 Audio_Wav_ConvertBlock( struct AudioWavResampler *state, const struct AudioWav *wav,
	U8 *output, U32 rate, U32 channels );

#endif
