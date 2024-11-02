#include "cnt_protocol.h"
#include <SDL3/SDL_assert.h>

void cnt_connection_packet_push(cnt_connection_packet *packet, cnt_connection_packet_type type, void *data, uint8_t length)
{
	SDL_assert(packet != nullptr);
	SDL_assert(packet->length + length + sizeof(cnt_connection_packet_header) <= packet->capacity &&
	           "Make sure the payload buffer is large enough!");

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

void cnt_connection_packet_write_uint32(cnt_connection_packet *packet, uint32_t value)
{
	constexpr size_t length = sizeof(value);
	SDL_assert(packet != nullptr);
	SDL_assert(packet->length + length + sizeof(cnt_connection_packet_header) <= packet->capacity &&
	           "Make sure the payload buffer is large enough!");

	packet->payload[packet->length + 0] = value & 0x000000ff;
	packet->payload[packet->length + 1] = value & 0x0000ff00;
	packet->payload[packet->length + 2] = value & 0x00ff0000;
	packet->payload[packet->length + 3] = value & 0xff000000;

	packet->length = packet->length + sizeof(value);
}

void cnt_connection_packet_read_uint32(cnt_connection_packet *packet, uint32_t *value)
{
	constexpr size_t length = sizeof(*value);
	SDL_assert(packet != nullptr);
	SDL_assert(packet->length + length + sizeof(cnt_connection_packet_header) <= packet->capacity &&
	           "Make sure the payload buffer is large enough!");

	*value |= (packet->payload[packet->index + 0] << 0);
	*value |= (packet->payload[packet->index + 1] << 8);
	*value |= (packet->payload[packet->index + 2] << 16);
	*value |= (packet->payload[packet->index + 3] << 24);

	packet->index = packet->index + sizeof(*value);
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

bool cnt_connection_is_little_endian()
{
	unsigned int x = 1;
	char *c = (char *)&x;
	return ((int)*c) == 1;
}