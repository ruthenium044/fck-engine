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
};

struct cnt_connection_accept
{
	uint32_t directed_secret;
};

struct cnt_connection_packet
{
	constexpr static size_t capacity = 64;

	uint8_t payload[capacity]; // enough for now?
	uint8_t length;
	uint8_t index;
};

void cnt_connection_packet_push(cnt_connection_packet *packet, cnt_connection_packet_type type, void *data, uint8_t length);
bool cnt_connection_packet_try_pop(cnt_connection_packet *packet, cnt_connection_packet_type *type, void **data, uint8_t *length);