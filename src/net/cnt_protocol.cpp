#include "cnt_protocol.h"
#include "ecs/fck_serialiser.h"
#include <SDL3/SDL_assert.h>

void fck_serialise(struct fck_serialiser *serialiser, cnt_connection_packet_header *header)
{
	fck_serialise(serialiser, &header->type);
	fck_serialise(serialiser, &header->length);
}

void cnt_connection_packet_push(cnt_connection_packet *packet, cnt_connection_packet_type type, void *data, uint16_t length)
{
	SDL_assert(packet != nullptr);
	SDL_assert(packet->length + length + sizeof(cnt_connection_packet_header) <= packet->write_capacity &&
	           "Make sure the payload buffer is large enough!");

	fck_serialiser seraliser;
	fck_serialiser_create(&seraliser, packet->payload, packet->write_capacity);
	fck_serialiser_byte_writer(&seraliser.self);

	cnt_connection_packet_header header;
	header.type = type;
	header.length = length;
	fck_serialise(&seraliser, &header);
	packet->length = seraliser.at;

	if (length != 0)
	{
		fck_serialise(&seraliser, (uint8_t *)data, length);
		packet->length = seraliser.at;
	}
}

bool cnt_connection_packet_try_pop(cnt_connection_recv_packet *packet, cnt_connection_packet_type *type, void **data, uint16_t *length)
{
	SDL_assert(packet != nullptr);

	if (packet->index >= packet->length)
	{
		return false;
	}

	fck_serialiser seraliser;
	fck_serialiser_create(&seraliser, packet->payload, packet->length);
	fck_serialiser_byte_reader(&seraliser.self);

	cnt_connection_packet_header header;
	fck_serialise(&seraliser, &header);

	*type = (cnt_connection_packet_type)header.type;
	*length = header.length;

	packet->index = seraliser.at;

	if (header.length != 0)
	{
		// Ehhh...
		*data = seraliser.data + seraliser.at;
		packet->index = seraliser.at + *length;
	}
	return true;
}

bool cnt_connection_is_little_endian()
{
	unsigned int x = 1;
	char *c = (char *)&x;
	return ((int)*c) == 1;
}