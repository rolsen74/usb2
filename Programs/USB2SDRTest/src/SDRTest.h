#ifndef USB2_SDR_TEST_H
#define USB2_SDR_TEST_H

#include <proto/usb2.h>

#define SDR_R820T_ADDRESS	0x34
#define SDR_CLOCK_HZ		28800000U
#define SDR_SAMPLE_RATE		1024000U
#define SDR_IQ_BYTES_PER_SECOND	( SDR_SAMPLE_RATE * 2 )
#define SDR_MAX_CAPTURE_SECONDS	5U
#define SDR_FM_AUDIO_RATE	48000U
#define SDR_FM_DECIMATION	4U
#define SDR_FM_MPX_RATE		( SDR_SAMPLE_RATE / SDR_FM_DECIMATION )
#define SDR_FM_BLOCK_PAIRS	32768U
#define SDR_IF_HZ		3570000U
#define SDR_FM_MIN_HZ		87500000U
#define SDR_FM_MAX_HZ		108000000U

struct SDRFMDecoder;

U8 SDR_FindBulkIn( const U8 *config, U32 size, U32 interface );
S32 SDR_Control( struct USB2_EPResource *control, U16 address, U16 index, U8 *data, U16 length );
S32 SDR_WriteRegister( struct USB2_EPResource *control, U16 address, U16 index, U16 value, U16 length );
S32 SDR_WriteDemod( struct USB2_EPResource *control, U8 page, U8 reg, U16 value, U16 length );
S32 SDR_SetI2CGate( struct USB2_EPResource *control, S32 enabled );
S32 SDR_PowerOn( struct USB2_EPResource *control );
S32 SDR_PowerOff( struct USB2_EPResource *control );
S32 SDR_ProbeTuner( struct USB2_EPResource *control, U8 *tunerAddress );
S32 SDR_R820T_Tune( struct USB2_EPResource *control, U32 frequency );
S32 SDR_R820T_Standby( struct USB2_EPResource *control );
S32 SDR_ConfigureIQ( struct USB2_EPResource *control );
S32 SDR_HoldFIFO( struct USB2_EPResource *control, S32 reset );
struct USB2_EPResource *SDR_OpenIQ( struct USB2_Register *reg, U8 endpoint, U32 frequency, U32 requests );
S32 SDR_CloseIQ( struct USB2_EPResource *control, struct USB2_EPResource *bulk );
S32 SDR_CaptureIQ( struct USB2_Register *reg, U8 endpoint, U32 frequency, U8 *samples, U32 bytes );
S32 SDR_SaveIQ( const char *path, const U8 *samples, U32 bytes );
S32 SDR_SaveFM( const char *path, const U8 *iq, U32 bytes );
S32 SDR_DecodeFM( const char *inputPath, const char *outputPath );
struct SDRFMDecoder *SDR_CreateFMDecoder( U32 audioRate );
// Consumes up to SDR_FM_BLOCK_PAIRS and returns the number of mono PCM16 frames.
U32 SDR_ProcessFM( struct SDRFMDecoder *decoder, const U8 *iq, U32 pairs, U8 *pcm );
S32 SDR_ListenFM( struct USB2_Register *reg, U8 endpoint, U32 frequency, U32 output );

#endif
