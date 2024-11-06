#include "cnt_protocol.h"
#include <SDL3/SDL_assert.h>

void cnt_connection_packet_push(cnt_connection_packet *packet, cnt_connection_packet_type type, void *data, uint16_t length)
{
	SDL_assert(packet != nullptr);
	SDL_assert(packet->length + length + sizeof(cnt_connection_packet_header) <= packet->write_capacity &&
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

bool cnt_connection_packet_try_pop(cnt_connection_recv_packet *packet, cnt_connection_packet_type *type, void **data, uint16_t *length)
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