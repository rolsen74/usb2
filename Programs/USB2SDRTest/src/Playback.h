#ifndef USB2_SDR_PLAYBACK_H
#define USB2_SDR_PLAYBACK_H

#include "../../../Drivers/Audio/inc/AudioDevice.h"

#define SDR_OUTPUT_AUTO (~0U)
#define SDR_PLAYBACK_BLOCKS 2U

struct SDRPlayback
{
	struct MsgPort *Port;
	struct IOStdReq *Request;
	struct AudioFormat Format;
	U8 *Staging;
	U8 *Playing;
	U32 StagedBytes;
	U32 StagedBlocks;
	U32 BlockBytes;
	U32 Unit;
	U32 SkippedBlocks;
	U32 ShortBytes;
	U32 DroppedSinceReply;
	S32 Opened;
	S32 Pending;
};

S32 SDR_OpenPlayback( struct SDRPlayback *play, U32 output );
void SDR_ClosePlayback( struct SDRPlayback *play );
S32 SDR_ServicePlayback( struct SDRPlayback *play );
S32 SDR_QueuePlayback( struct SDRPlayback *play, const U8 *pcm, U32 frames );

#endif
