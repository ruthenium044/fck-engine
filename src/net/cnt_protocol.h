#include "SDL3/SDL_assert.h"
#include <SDL3/SDL_stdinc.h>

enum cnt_connection_packet_type
{
	CNT_CONNECTION_PACKET_TYPE_REQUEST,
	CNT_CONNECTION_PACKET_TYPE_ACCEPT,
	CNT_CONNECTION_PACKET_TYPE_REJECT,
	CNT_CONNECTION_PACKET_TYPE_OK
};

static constexpr uint32_t CNT_PROTOCOL_ID = 'FCK';      // NOLINT
static constexpr uint32_t CNT_PROTOCOL_VERSION = '0.1'; // NOLINT

struct cnt_connection_packet_header
{
	uint8_t type;   // 255 tyoes
	uint8_t length; // 255 max bytes per packet segment
};

struct cnt_connection_request
{
	uint32_t protocol;
	uint32_t version;
};

struct cnt_connection_packet
{
	constexpr static size_t capacity = 64;

	uint8_t payload[capacity]; // enough for now?
	uint8_t length;
	uint8_t index;
};

void cnt_connection_packet_push(cnt_connection_packet *packet, cnt_connection_packet_type type, void *data, uint8_t length)
{
	SDL_assert(packet != nullptr);
	SDL_assert(packet->length + length + sizeof(cnt_connection_packet_header) <= packet->capacity &&
	           "Make sure the payload buffer is large enough!");
	SDL_assert(bool(data) == bool(length) && "Either both null or none");

	cnt_connection_packet_header header;
	header.type = type;
	header.length = length;

	SDL_memcpy(packet->payload + packet->length, &header, sizeof(header));
	packet->length = packet->length + sizeof(header);

	if (length != 0)
	{
		SDL_memcpy(packet->payload + packet->length, data, length);
		packet->length = packet->length + length;
	}
}

bool cnt_connection_packet_try_pop(cnt_connection_packet *packet, cnt_connection_packet_type *type, void **data, uint8_t *length)
{
	SDL_assert(packet != nullptr);

	if (packet->index >= packet->length)
	{
		return false;
	}

	cnt_connection_packet_header *header = (cnt_connection_packet_header *)(packet->payload + packet->index);
	*type = (cnt_connection_packet_type)header->type;
	*length = header->length;

	packet->index = packet->index + sizeof(*header);

	if (header->length != 0)
	{
		*data = packet->payload + packet->index;
		packet->index = packet->index + header->length;
	}
	return true;
}