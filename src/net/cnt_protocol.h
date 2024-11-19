#include <SDL3/SDL_stdinc.h>

enum cnt_connection_packet_type
{
	CNT_CONNECTION_PACKET_TYPE_REQUEST,
	CNT_CONNECTION_PACKET_TYPE_ACCEPT,
	CNT_CONNECTION_PACKET_TYPE_REJECT,
	CNT_CONNECTION_PACKET_TYPE_OK,
	CNT_CONNECTION_PACKET_TYPE_DATA
};

static constexpr uint32_t CNT_PROTOCOL_ID = 'FCK';      // NOLINT
static constexpr uint32_t CNT_PROTOCOL_VERSION = '0.1'; // NOLINT

// send handshake packet
// - Constructed via cnt_connection_packet with a static size of 64 bytes

// Send data packet:
// - Header on top, manually constructed by appending buffer

// cnt_connection_recv_packet generically reads handshake and data packets alike!

struct cnt_connection_packet_header
{
	uint8_t type;    // 255 tyoes
	uint16_t length; // 255 max bytes per packet segment
};

struct cnt_connection_request
{
	uint32_t protocol;
	uint32_t version;
	uint32_t suggested_secret;
	uint8_t is_little_endian;
};

struct cnt_connection_accept
{
	uint32_t directed_secret;
};

struct cnt_connection_packet
{
	constexpr static size_t write_capacity = 60;

	uint16_t length;
	uint16_t index;
	uint8_t payload[write_capacity]; // enough for now?
};

struct cnt_connection_recv_packet
{
	uint16_t length;
	uint16_t index;
	uint8_t *payload; // enough for now?
};

void cnt_connection_packet_push(cnt_connection_packet *packet, cnt_connection_packet_type type, void *data, uint16_t length);
bool cnt_connection_packet_try_pop(cnt_connection_recv_packet *packet, cnt_connection_packet_type *type, void **data, uint16_t *length);

void fck_serialise(struct fck_serialiser *serialiser, cnt_connection_packet_header *header);

bool cnt_connection_is_little_endian();
