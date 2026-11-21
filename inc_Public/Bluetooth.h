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
