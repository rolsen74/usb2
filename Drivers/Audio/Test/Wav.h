#ifndef USB2_AUDIO_TEST_WAV_H
#define USB2_AUDIO_TEST_WAV_H

struct AudioWav
{
	U8 *Samples;
	U32 Frames;
	U32 Rate;
	U32 Channels;
};

S32 Audio_Wav_Parse( U8 *data, U32 length, struct AudioWav *wav );
void Audio_Wav_Convert( const struct AudioWav *wav, U8 *output, U32 frames, U32 rate, U32 channels );

#endif
