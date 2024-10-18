#include <SDL3/SDL_stdinc.h>

enum cnt_connection_packet_type
{
	CNT_CONNECTION_PACKET_TYPE_REQUEST,
	CNT_CONNECTION_PACKET_TYPE_REQUESTING,
	CNT_CONNECTION_PACKET_TYPE_ACCEPT,
	CNT_CONNECTION_PACKET_TYPE_REJECT,
	CNT_CONNECTION_PACKET_TYPE_OK
};

static constexpr uint32_t CNT_PROTOCOL_ID = 'FCK';      // NOLINT
static constexpr uint32_t CNT_PROTOCOL_VERSION = '0.1'; // NOLINT

struct cnt_connection_packet
{
	uint32_t type;
	uint32_t length;
};

struct cnt_connection_request
{
	cnt_connection_packet base;
	uint32_t protocol;
	uint32_t version;
};

struct cnt_connection_accept
{
	cnt_connection_packet base;
};

struct cnt_connection_reject
{
	cnt_connection_packet base;
};

struct cnt_connection_ok
{
	cnt_connection_packet base;
};
