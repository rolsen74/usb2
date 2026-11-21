#ifndef USB2_BLUETOOTH_HOST_H
#define USB2_BLUETOOTH_HOST_H

#include <stdint.h>

// A transport can report this when no event data arrived before its timeout.
#define BT_HCI_TRANSPORT_TIMEOUT (-1000)

struct BTHCITransport
{
	void *Context;
	int32_t (*StartCommand)( void *context, const uint8_t *command, uint32_t length );
	// Each chunk remains valid until the next transport call.
	int32_t (*ReadEventChunk)( void *context, const uint8_t **data, uint32_t *length );
};

struct BTACLTransport
{
	struct BTHCITransport HCI;
	int32_t (*WriteACL)( void *context, const uint8_t *packet, uint32_t length );
	int32_t (*ReadACLChunk)( void *context, const uint8_t **data, uint32_t *length );
};

enum BTHCIResult
{
	BT_HCI_OK = 0,
	BT_HCI_WRITE_ERROR,
	BT_HCI_READ_ERROR,
	BT_HCI_COMMAND_ERROR,
	BT_HCI_INVALID_REPLY,
	BT_HCI_NO_CONTROLLER,
	BT_HCI_NO_MEMORY,
	BT_HCI_UNSUPPORTED,
	BT_HCI_INVALID_ARGUMENT,
	BT_HCI_NO_SAVED_KEY,
	BT_HCI_NO_NEW_KEY,
	BT_HCI_BOND_IO_ERROR,
	BT_HCI_TIMER_ERROR,
	BT_ACL_WRITE_ERROR,
	BT_ACL_READ_ERROR,
	BT_L2CAP_REJECTED,
	BT_SDP_ERROR,
	BT_RFCOMM_REJECTED,
	BT_RFCOMM_TX_LIMIT,
	BT_RFCOMM_FILE_ERROR,
	BT_RFCOMM_NEEDS_CREDITS,
	BT_RFCOMM_CANCELLED,
	BT_OBEX_REJECTED,
	BT_OBEX_NAME_TOO_LONG,
	BT_OBEX_SIZE_LIMIT,
	BT_LE_NOT_SUPPORTED,
	BT_LE_CANCELLED,
	BT_OBEX_NO_SERVICE,
	BT_A2DP_INVALID_WAVE,
	BT_A2DP_NO_SINK,
	BT_A2DP_REJECTED,
	BT_HID_REJECTED,
	BT_HID_DISCONNECTED,
	BT_HID_INPUT_ERROR,
};

struct BTHCIError
{
	uint16_t Opcode;
	int32_t TransportError;
	uint8_t Status;
};

struct BTControllerInfo
{
	uint8_t HCIVersion;
	uint16_t HCIRevision;
	uint8_t LMPVersion;
	uint16_t Manufacturer;
	uint16_t LMPSubversion;
	uint8_t Address[6];
};

struct BTInquiryDevice
{
	uint8_t Address[6];
	uint8_t PageScanRepetitionMode;
	uint16_t ClockOffset;
	uint32_t ClassOfDevice;
};

struct BTInquiryResult
{
	struct BTInquiryDevice *Devices;
	uint32_t Capacity;
	uint32_t Count;
	uint8_t Truncated;
};

#define BT_PIN_MAX 16

struct BTPairRequest
{
	uint8_t Address[6];
	uint8_t PINLength;
	uint8_t PIN[BT_PIN_MAX];
};

struct BTBondRequest
{
	struct BTPairRequest Peer;
	uint8_t Reconnect;
	uint8_t SentSavedKey;
};

#define BT_REMOTE_NAME_MAX 248

struct BTRemoteName
{
	uint8_t Address[6];
	char Name[BT_REMOTE_NAME_MAX + 1];
};

#define BT_VISIBLE_MAX_SECONDS 120

struct BTVisibility
{
	uint32_t Seconds;
	char Name[BT_REMOTE_NAME_MAX + 1];
};

struct BTServiceRecord
{
	uint32_t Handle;
	uint8_t UUID[16];
	uint8_t UUIDLength;
	char Name[64];
	uint8_t RFCOMMChannel;
};

struct BTServiceQuery
{
	uint8_t Address[6];
	struct BTServiceRecord *Records;
	uint32_t Capacity;
	uint32_t Count;
	uint16_t Status;
	uint8_t Truncated;
};

enum BTRFCOMMStage
{
	BT_RFCOMM_STAGE_LINK,
	BT_RFCOMM_STAGE_L2CAP,
	BT_RFCOMM_STAGE_MULTIPLEXER,
	BT_RFCOMM_STAGE_PARAMETERS,
	BT_RFCOMM_STAGE_CHANNEL,
	BT_RFCOMM_STAGE_MODEM,
	BT_RFCOMM_STAGE_CLOSE,
	BT_RFCOMM_STAGE_DATA,
};

struct BTRFCOMMTest
{
	uint8_t Address[6];
	uint8_t Channel;
	uint8_t Stage;
};

#define BT_RFCOMM_TEST_DATA_MAX 512

struct BTRFCOMMTransfer
{
	struct BTRFCOMMTest Peer;
	const uint8_t *Data;
	uint32_t Length;
	uint32_t Sent;
	uint8_t *Reply;
	uint32_t ReplyCapacity;
	uint32_t ReplyLength;
	uint8_t ReplyTruncated;
};

struct BTRFCOMMFile
{
	struct BTRFCOMMTest Peer;
	const char *Path;
	uint64_t Sent;
	int32_t FileError;
	uint32_t Cancel;
};

struct BTRFCOMMCapture
{
	struct BTRFCOMMTest Peer;
	const char *Path;
	uint64_t Expected;
	uint64_t Received;
	int32_t FileError;
	uint32_t Cancel;
};

struct BTObjectPush
{
	struct BTRFCOMMTest Peer;
	const char *Path;
	uint32_t Sent;
	int32_t FileError;
	uint32_t Cancel;
	uint8_t Response;
};

#define BT_LE_DATA_MAX 31
#define BT_LE_NAME_MAX ( BT_LE_DATA_MAX - 2 )
#define BT_LE_MAX_SECONDS 120
#define BT_LE_RSSI_UNKNOWN 127

enum BTLEMode
{
	BT_LE_SCAN_ACTIVE = 0,
	BT_LE_SCAN_PASSIVE,
	BT_LE_ADVERTISE,
};

struct BTLEDevice
{
	uint8_t Address[6];
	uint8_t AddressType;
	int8_t RSSI;
	uint8_t AdvertisingType;
	uint8_t AdvertisingLength;
	uint8_t ScanResponseLength;
	uint8_t AdvertisingData[BT_LE_DATA_MAX];
	uint8_t ScanResponseData[BT_LE_DATA_MAX];
	uint8_t CompleteName;
	char Name[BT_LE_NAME_MAX + 1];
};

struct BTLERequest
{
	enum BTLEMode Mode;
	struct BTLEDevice *Devices;
	uint32_t Capacity;
	uint32_t Count;
	uint32_t Seconds;
	uint32_t Cancel;
	uint8_t Truncated;
	char Name[BT_LE_NAME_MAX + 1];
};

struct BTAudioPlayback
{
	const char *Path;
	uint8_t Address[6];
	uint8_t Signal;
	uint8_t Status;
	uint32_t Cancel;
	uint32_t Rate;
	uint32_t FramesSent;
	int32_t FileError;
};

struct BTAudioPCM
{
	struct BTAudioPlayback Audio;
	void *Context;
	// Pulls interleaved stereo S16LE from the controller task until cancelled.
	int32_t (*Read)( void *context, uint8_t *data, uint32_t length );
};

#define BT_HID_BOOT_KEYBOARD 1
#define BT_HID_BOOT_MOUSE 2
#define BT_HID_BOOT_PAYLOAD_MAX 45
#define BT_HID_MAX_SECONDS 120

struct BTHIDReport
{
	uint8_t ID;
	uint8_t Length;
	uint8_t Data[BT_HID_BOOT_PAYLOAD_MAX];
};

struct BTHIDMonitor
{
	struct BTHIDReport *Reports;
	uint8_t Address[6];
	uint8_t Handshake;
	uint32_t Seconds;
	uint32_t Capacity;
	uint32_t Count;
	uint32_t Cancel;
};

// The callbacks run on the controller task. Stop follows each successful Start.
struct BTHIDInput
{
	struct BTHIDMonitor Monitor;
	void *Context;
	int32_t (*Start)( void *context );
	int32_t (*Report)( void *context, const struct BTHIDReport *report );
	void (*Stop)( void *context );
};

enum BTHCIResult BT_ExchangeRFCOMM( const struct BTACLTransport *transport, struct BTRFCOMMTransfer *transfer,
	struct BTHCIError *error );
enum BTHCIResult BT_TestRFCOMM( const struct BTACLTransport *transport, struct BTRFCOMMTest *test,
	struct BTHCIError *error );
enum BTHCIResult BT_BrowseServices( const struct BTACLTransport *transport, struct BTServiceQuery *query,
	struct BTHCIError *error );
enum BTHCIResult BT_L2CAPEcho( const struct BTACLTransport *transport, const uint8_t *address,
	struct BTHCIError *error );
enum BTHCIResult BT_ReadRemoteName( const struct BTHCITransport *transport,
	struct BTRemoteName *remote, struct BTHCIError *error );
enum BTHCIResult BT_Pair( const struct BTHCITransport *transport,
	const struct BTPairRequest *pair, struct BTHCIError *error );
enum BTHCIResult BT_Inquiry( const struct BTHCITransport *transport,
	struct BTInquiryResult *inquiry, struct BTHCIError *error );
enum BTHCIResult BT_HCICommand( const struct BTHCITransport *transport, uint16_t opcode,
	uint8_t *reply, uint32_t replyLength, struct BTHCIError *error );
enum BTHCIResult BT_ResetController( const struct BTHCITransport *transport, struct BTHCIError *error );
enum BTHCIResult BT_ReadControllerInfo( const struct BTHCITransport *transport,
	struct BTControllerInfo *info, struct BTHCIError *error );

#endif
