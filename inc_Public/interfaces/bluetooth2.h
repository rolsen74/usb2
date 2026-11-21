#ifndef INTERFACES_BLUETOOTH2_H
#define INTERFACES_BLUETOOTH2_H

#include <exec/interfaces.h>
#include <exec/ports.h>
#include "Bluetooth.h"

#define BLUETOOTH2_NAME "bluetooth2.device"
#define BLUETOOTH2_INTERFACE_VERSION 3

struct BTControllerLocation
{
	uint16_t VendorID;
	uint16_t ProductID;
	uint8_t EventEndpoint;
};

// The driver worker owns the request port and services requests on its task.
struct BTControllerProbe
{
	struct Message Message;
	struct BTControllerInfo *Info;
	struct BTHCIError *Error;
	enum BTHCIResult Result;
	struct BTInquiryResult *Inquiry;
	const struct BTPairRequest *Pair;
	struct BTBondRequest *Bond;
	struct BTRemoteName *RemoteName;
	const struct BTVisibility *Visibility;
	const uint8_t *EchoAddress;
	struct BTServiceQuery *Services;
	struct BTRFCOMMTest *RFCOMM;
	struct BTRFCOMMTransfer *Serial;
	struct BTRFCOMMFile *SerialFile;
	struct BTRFCOMMCapture *SerialCapture;
	struct BTObjectPush *ObjectPush;
	struct BTLERequest *LE;
	struct BTAudioPlayback *Audio;
	struct BTAudioPCM *PCM;
};

struct Bluetooth2IFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2IFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2IFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2IFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2IFace *Self );

	enum BTHCIResult APICALL (*ResetController)( struct Bluetooth2IFace *Self,
		const struct BTHCITransport *transport, struct BTHCIError *error );
	enum BTHCIResult APICALL (*ReadControllerInfo)( struct Bluetooth2IFace *Self,
		const struct BTHCITransport *transport, struct BTControllerInfo *info,
		struct BTHCIError *error );

	int32_t APICALL (*AddController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
	void APICALL (*RemoveController)( struct Bluetooth2IFace *Self, uint32_t unit, struct MsgPort *port );
	enum BTHCIResult APICALL (*ProbeController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTControllerLocation *location, struct BTControllerInfo *info, struct BTHCIError *error );
	enum BTHCIResult APICALL (*DiscoverController)( struct Bluetooth2IFace *Self, uint32_t unit,
		struct BTControllerLocation *location, struct BTInquiryResult *inquiry, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Inquiry)( struct Bluetooth2IFace *Self,
		const struct BTHCITransport *transport, struct BTInquiryResult *inquiry, struct BTHCIError *error );
	int32_t APICALL (*AddDiscoveryController)( struct Bluetooth2IFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#define BLUETOOTH2_PAIRING_INTERFACE "pairing"
#define BLUETOOTH2_PAIRING_VERSION 1

struct Bluetooth2PairingIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2PairingIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2PairingIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2PairingIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2PairingIFace *Self );

	enum BTHCIResult APICALL (*PairController)( struct Bluetooth2PairingIFace *Self, uint32_t unit,
		const struct BTPairRequest *pair, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Pair)( struct Bluetooth2PairingIFace *Self,
		const struct BTHCITransport *transport, const struct BTPairRequest *pair, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2PairingIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#define BLUETOOTH2_BONDING_INTERFACE "bonding"
#define BLUETOOTH2_BONDING_VERSION 1

struct Bluetooth2BondingIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2BondingIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2BondingIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2BondingIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2BondingIFace *Self );

	enum BTHCIResult APICALL (*BondController)( struct Bluetooth2BondingIFace *Self, uint32_t unit,
		struct BTBondRequest *bond, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Bond)( struct Bluetooth2BondingIFace *Self,
		const struct BTHCITransport *transport, struct BTBondRequest *bond, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2BondingIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#define BLUETOOTH2_NAMES_INTERFACE "names"
#define BLUETOOTH2_NAMES_VERSION 1

struct Bluetooth2NamesIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2NamesIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2NamesIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2NamesIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2NamesIFace *Self );

	enum BTHCIResult APICALL (*ReadName)( struct Bluetooth2NamesIFace *Self, uint32_t unit,
		struct BTRemoteName *remote, struct BTHCIError *error );
	enum BTHCIResult APICALL (*ReadRemoteName)( struct Bluetooth2NamesIFace *Self,
		const struct BTHCITransport *transport, struct BTRemoteName *remote, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2NamesIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#define BLUETOOTH2_VISIBILITY_INTERFACE "visibility"
#define BLUETOOTH2_VISIBILITY_VERSION 1

struct Bluetooth2VisibilityIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2VisibilityIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2VisibilityIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2VisibilityIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2VisibilityIFace *Self );

	enum BTHCIResult APICALL (*MakeVisible)( struct Bluetooth2VisibilityIFace *Self, uint32_t unit,
		const struct BTVisibility *visibility, struct BTHCIError *error );
	enum BTHCIResult APICALL (*RunVisible)( struct Bluetooth2VisibilityIFace *Self,
		const struct BTHCITransport *transport, const struct BTVisibility *visibility, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2VisibilityIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#define BLUETOOTH2_L2CAP_INTERFACE "l2cap"
#define BLUETOOTH2_L2CAP_VERSION 1

struct Bluetooth2L2CAPIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2L2CAPIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2L2CAPIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2L2CAPIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2L2CAPIFace *Self );

	enum BTHCIResult APICALL (*EchoController)( struct Bluetooth2L2CAPIFace *Self, uint32_t unit,
		const uint8_t *address, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Echo)( struct Bluetooth2L2CAPIFace *Self,
		const struct BTACLTransport *transport, const uint8_t *address, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2L2CAPIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#define BLUETOOTH2_SDP_INTERFACE "sdp"
#define BLUETOOTH2_SDP_VERSION 1

struct Bluetooth2SDPIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2SDPIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2SDPIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2SDPIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2SDPIFace *Self );

	enum BTHCIResult APICALL (*BrowseController)( struct Bluetooth2SDPIFace *Self, uint32_t unit,
		struct BTServiceQuery *query, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Browse)( struct Bluetooth2SDPIFace *Self,
		const struct BTACLTransport *transport, struct BTServiceQuery *query, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2SDPIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#define BLUETOOTH2_RFCOMM_INTERFACE "rfcomm"
#define BLUETOOTH2_RFCOMM_VERSION 1

struct Bluetooth2RFCOMMIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2RFCOMMIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2RFCOMMIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2RFCOMMIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2RFCOMMIFace *Self );

	enum BTHCIResult APICALL (*TestController)( struct Bluetooth2RFCOMMIFace *Self, uint32_t unit,
		struct BTRFCOMMTest *test, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Test)( struct Bluetooth2RFCOMMIFace *Self,
		const struct BTACLTransport *transport, struct BTRFCOMMTest *test, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2RFCOMMIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#define BLUETOOTH2_SERIAL_INTERFACE "serial"
#define BLUETOOTH2_SERIAL_VERSION 1

struct Bluetooth2SerialIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2SerialIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2SerialIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2SerialIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2SerialIFace *Self );

	enum BTHCIResult APICALL (*ExchangeController)( struct Bluetooth2SerialIFace *Self, uint32_t unit,
		struct BTRFCOMMTransfer *transfer, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Exchange)( struct Bluetooth2SerialIFace *Self,
		const struct BTACLTransport *transport, struct BTRFCOMMTransfer *transfer, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2SerialIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#define BLUETOOTH2_SERIALFILE_INTERFACE "serialfile"
#define BLUETOOTH2_SERIALFILE_VERSION 1

struct Bluetooth2SerialFileIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2SerialFileIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2SerialFileIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2SerialFileIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2SerialFileIFace *Self );

	enum BTHCIResult APICALL (*SendController)( struct Bluetooth2SerialFileIFace *Self, uint32_t unit,
		struct BTRFCOMMFile *file, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Send)( struct Bluetooth2SerialFileIFace *Self,
		const struct BTACLTransport *transport, struct BTRFCOMMFile *file, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2SerialFileIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#define BLUETOOTH2_CAPTURE_INTERFACE "serialcapture"
#define BLUETOOTH2_CAPTURE_VERSION 1

struct Bluetooth2SerialCaptureIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2SerialCaptureIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2SerialCaptureIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2SerialCaptureIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2SerialCaptureIFace *Self );

	enum BTHCIResult APICALL (*ReceiveController)( struct Bluetooth2SerialCaptureIFace *Self, uint32_t unit,
		struct BTRFCOMMCapture *capture, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Receive)( struct Bluetooth2SerialCaptureIFace *Self,
		const struct BTACLTransport *transport, struct BTRFCOMMCapture *capture, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2SerialCaptureIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#define BLUETOOTH2_OBEX_INTERFACE "objectpush"
#define BLUETOOTH2_OBEX_VERSION 1

struct Bluetooth2ObjectPushIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2ObjectPushIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2ObjectPushIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2ObjectPushIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2ObjectPushIFace *Self );

	enum BTHCIResult APICALL (*PushController)( struct Bluetooth2ObjectPushIFace *Self, uint32_t unit,
		struct BTObjectPush *push, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Push)( struct Bluetooth2ObjectPushIFace *Self,
		const struct BTACLTransport *transport, struct BTObjectPush *push, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2ObjectPushIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#define BLUETOOTH2_LE_INTERFACE "le"
#define BLUETOOTH2_LE_VERSION 1

struct Bluetooth2LEIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2LEIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2LEIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2LEIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2LEIFace *Self );

	enum BTHCIResult APICALL (*RunController)( struct Bluetooth2LEIFace *Self, uint32_t unit,
		struct BTLERequest *request, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Run)( struct Bluetooth2LEIFace *Self,
		const struct BTHCITransport *transport, struct BTLERequest *request, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2LEIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location, uint32_t acl );
};

#define BLUETOOTH2_A2DP_INTERFACE "a2dp"
#define BLUETOOTH2_A2DP_VERSION 1

struct Bluetooth2A2DPIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2A2DPIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2A2DPIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2A2DPIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2A2DPIFace *Self );

	enum BTHCIResult APICALL (*PlayController)( struct Bluetooth2A2DPIFace *Self, uint32_t unit,
		struct BTAudioPlayback *audio, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Play)( struct Bluetooth2A2DPIFace *Self, const struct BTACLTransport *transport,
		struct BTAudioPlayback *audio, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2A2DPIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#define BLUETOOTH2_PCM_INTERFACE "a2dp-pcm"
#define BLUETOOTH2_PCM_VERSION 1

struct Bluetooth2PCMIFace
{
	struct InterfaceData Data;

	ULONG APICALL (*Obtain)( struct Bluetooth2PCMIFace *Self );
	ULONG APICALL (*Release)( struct Bluetooth2PCMIFace *Self );
	void APICALL (*Expunge)( struct Bluetooth2PCMIFace *Self );
	struct Interface *APICALL (*Clone)( struct Bluetooth2PCMIFace *Self );

	enum BTHCIResult APICALL (*PlayController)( struct Bluetooth2PCMIFace *Self, uint32_t unit,
		struct BTAudioPCM *pcm, struct BTHCIError *error );
	enum BTHCIResult APICALL (*Play)( struct Bluetooth2PCMIFace *Self, const struct BTACLTransport *transport,
		struct BTAudioPCM *pcm, struct BTHCIError *error );
	int32_t APICALL (*AddController)( struct Bluetooth2PCMIFace *Self, struct MsgPort *port,
		const struct BTControllerLocation *location );
};

#endif
