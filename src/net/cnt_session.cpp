#include "cnt_protocol.h"
#include "net/cnt_session.h"
#include "shared/fck_checks.h"
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_atomic.h>
#include <stdlib.h>

#if _WIN32
#include <winsock2.h>
static SDL_AtomicInt wsa_reference_counter = {0};
#endif // _WIN32

void cnt_socket_memory_buffer_alloc(cnt_socket_memory_buffer *memory)
{
	SDL_assert(memory != nullptr);
	SDL_zerop(memory);
	memory->data = (uint8_t *)SDL_malloc(cnt_socket_memory_buffer::capacity);
}

void cnt_socket_memory_buffer_free(cnt_socket_memory_buffer *memory)
{
	SDL_assert(memory != nullptr);
	SDL_free(memory->data);
	SDL_zerop(memory);
}

void cnt_socket_memory_buffer_clear(cnt_socket_memory_buffer *memory)
{
	SDL_assert(memory != nullptr);

	memory->length = 0;
}

void cnt_socket_memory_buffer_append(cnt_socket_memory_buffer *memory, uint8_t *data, size_t size)
{
	SDL_assert(memory != nullptr);

	if (memory->length + size > cnt_socket_memory_buffer::capacity)
	{
		memory->generation = memory->generation + 1;
		memory->length = 0;
	}

	SDL_memcpy(memory->data + memory->length, data, size);

	memory->length = memory->length + size;
}

void cnt_session_alloc(cnt_session *session, cnt_socket_id socket_capacity, cnt_address_id address_capacity,
                       cnt_connection_id connection_capacity, uint64_t tick_rate)
{
	SDL_assert(session != nullptr);
	SDL_zerop(session);

	session->tick_rate = tick_rate;

	fck_sparse_list_alloc(&session->addresses, address_capacity);
	fck_sparse_list_alloc(&session->connections, connection_capacity);
	fck_sparse_list_alloc(&session->sockets, socket_capacity);

	fck_sparse_array_alloc(&session->recv_buffer, socket_capacity);
	fck_queue_alloc(&session->recv_frames, cnt_session::maximum_frame_capacity);

	session->temp_recv_buffer = (uint8_t *)SDL_malloc(cnt_socket_memory_buffer::capacity);
	session->temp_send_buffer = (uint8_t *)SDL_malloc(cnt_socket_memory_buffer::capacity);

	fck_queue_alloc(&session->new_connections, ~0); // Full capacity 255

#if _WIN32
	// TODO: Fix this non-sense up.
	int reference_counter = SDL_GetAtomicInt(&wsa_reference_counter);
	if (reference_counter == 0)
	{
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
	}
	SDL_AtomicIncRef(&wsa_reference_counter);
#endif // _WIN32
}

void cnt_session_free(cnt_session *session)
{
	SDL_assert(session != nullptr);

	for (cnt_socket_memory_buffer *memory : &session->recv_buffer.dense)
	{
		cnt_socket_memory_buffer_free(memory);
	}

	fck_sparse_list_free(&session->addresses);
	fck_sparse_list_free(&session->connections);
	fck_sparse_list_free(&session->sockets);

	fck_sparse_array_free(&session->recv_buffer);

	SDL_free(session->temp_recv_buffer);
	SDL_free(session->temp_send_buffer);

	fck_queue_free(&session->recv_frames);
	fck_queue_free(&session->new_connections);

	SDL_zerop(session);

#if _WIN32
	SDL_AtomicDecRef(&wsa_reference_counter);
	int reference_counter = SDL_GetAtomicInt(&wsa_reference_counter);
	SDL_assert(reference_counter >= 0);

	if (reference_counter == 0)
	{
		WSACleanup();
	}
#endif // _WIN32
}

const char *cnt_address_to_string(cnt_session *session, cnt_address_id id)
{
	return fck_sparse_list_view(&session->addresses, id)->debug;
}

const char *cnt_socket_to_string(cnt_session *session, cnt_socket_id id)
{
	cnt_address_id address_id = fck_sparse_list_view(&session->sockets, id)->address;
	return fck_sparse_list_view(&session->addresses, address_id)->debug;
}

cnt_connection *cnt_session_connection_find_with_socket(cnt_session *session, cnt_socket_id const *socket_id, cnt_address *address,
                                                        cnt_connection_id *connection_id)
{
	for (fck_item<cnt_connection_id, cnt_connection> item : &session->connections)
	{
		cnt_connection *connection = item.value;
		if (connection->source == *socket_id)
		{
			cnt_address *addr = &fck_sparse_list_view(&session->addresses, connection->destination)->address;
			if (addr != nullptr && cnt_address_equals(addr, address))
			{
				if (connection_id != nullptr)
				{
					*connection_id = *item.index;
				}
				return connection;
			}
		}
	}
	return nullptr;
}

cnt_address_handle cnt_session_address_create(cnt_session *session, cnt_address *addr)
{
	if (addr == nullptr)
	{
		return {CNT_ADDRESS_INVALID_ID};
	}
	for (fck_item<cnt_address_id, cnt_address_data> item : &session->addresses)
	{
		cnt_address *address = &item.value->address;
		if (cnt_address_equals(addr, address))
		{
			return {*item.index};
		}
	}

	cnt_address_data data;
	data.address = *addr;

	cnt_address_as_string(addr, data.debug, sizeof(data.debug));

	return {fck_sparse_list_add(&session->addresses, &data)};
}

cnt_address_handle cnt_session_address_create(cnt_session *session, char const *ip, uint16_t port)
{
	cnt_address addr = cnt_address_from_string(ip, port);
	return cnt_session_address_create(session, &addr);
}

cnt_socket_handle cnt_session_socket_create(cnt_session *session, char const *ip, uint16_t port)
{
	cnt_socket sock = cnt_socket_create(ip, port);
	if (sock == -1)
	{
		return {CNT_SOCKET_INVALID_ID};
	}
	// Just to keep track of self ip
	cnt_address_handle address_handle = cnt_session_address_create(session, ip, port);

	cnt_socket_data data;
	data.socket = sock;
	data.address = address_handle.id;

	cnt_socket_id socket_id = fck_sparse_list_add(&session->sockets, &data);
	cnt_socket_memory_buffer socket_send_memory;
	cnt_socket_memory_buffer socket_recv_memory;

	cnt_socket_memory_buffer_alloc(&socket_send_memory);
	cnt_socket_memory_buffer_alloc(&socket_recv_memory);

	fck_sparse_array_emplace(&session->recv_buffer, socket_id, &socket_recv_memory);

	return {socket_id};
}

void cnt_session_connect(cnt_session *session, cnt_socket_handle const *socket, cnt_address_handle const *address)
{
	cnt_connection connection;
	SDL_zero(connection);

	connection.source = socket->id;
	connection.destination = address->id;
	connection.last_timestamp = 0;
	for (fck_item<cnt_address_id, cnt_connection> item : &session->connections)
	{
		cnt_connection *current = item.value;
		if (current->destination == connection.destination && current->source && connection.source)
		{
			return;
		}
	}
	connection.state = CNT_CONNECTION_STATE_CONNECTING;
	connection.flags = CNT_CONNECTION_FLAG_CLIENT;
	cnt_connection_id connection_id = fck_sparse_list_add(&session->connections, &connection);
}

bool cnt_session_fetch_data(cnt_session *session, cnt_connection *connection, cnt_socket_data *sock, cnt_address_data *address)
{
	cnt_socket_data *sock_ptr = fck_sparse_list_view(&session->sockets, connection->source);
	cnt_address_data *address_ptr = fck_sparse_list_view(&session->addresses, connection->destination);
	CHECK_WARNING(sock_ptr != nullptr, "Cannot find socket", return false);
	CHECK_WARNING(address_ptr != nullptr, "Cannot find address", return false);

	SDL_assert(sock_ptr->socket != -1);
	// TODO: Find a good way to identify invalid IP
	// SDL_assert(address_ptr->address != nullptr);

	*sock = *sock_ptr;
	*address = *address_ptr;
	return true;
}

void cnt_session_tick_connecting(cnt_session *session, cnt_connection *connection, fck_milliseconds time, fck_milliseconds delta_time)
{
	cnt_socket_data sock;
	cnt_address_data address;

	if (!cnt_session_fetch_data(session, connection, &sock, &address))
	{
		return;
	}

	cnt_connection_packet packet;
	SDL_zero(packet);

	cnt_connection_request request;
	request.protocol = CNT_PROTOCOL_ID;
	request.version = CNT_PROTOCOL_VERSION;
	request.is_little_endian = cnt_connection_is_little_endian();
	request.suggested_secret = 0;

	cnt_connection_packet_push(&packet, CNT_CONNECTION_PACKET_TYPE_REQUEST, &request, sizeof(request));

	cnt_sendto(sock.socket, packet.payload, packet.length, &address.address);

	connection->state = CNT_CONNECTION_STATE_REQUEST_OUTGOING;
	connection->last_timestamp = time;

	SDL_Log("%llu, %s connects to %s", (uint64_t)session, cnt_socket_to_string(session, connection->source), address.debug);
}

void cnt_session_tick_accepts(cnt_session *session, cnt_connection *connection, fck_milliseconds time, fck_milliseconds delta_time)
{
	cnt_socket_data sock;
	cnt_address_data address;

	if (!cnt_session_fetch_data(session, connection, &sock, &address))
	{
		return;
	}

	cnt_connection_packet packet;
	SDL_zero(packet);

	cnt_connection_accept accept;
	SDL_zero(accept);

	if (connection->secret != 0)
	{
		connection->secret = accept.directed_secret;
	}
	else
	{
		int random_nunber = rand(); // NO LINT
		uint32_t secret_base = *(uint32_t *)&random_nunber;
		connection->secret = secret_base | (secret_base << 16);
	}

	accept.directed_secret = connection->secret;
	cnt_connection_packet_push(&packet, CNT_CONNECTION_PACKET_TYPE_ACCEPT, &accept, sizeof(accept));
	cnt_sendto(sock.socket, packet.payload, packet.length, &address.address);

	connection->state = CNT_CONNECTION_STATE_WAITING_FOR_ACKNOWLDGEMENT;
	connection->last_timestamp = time;

	SDL_Log("%llu, %s accepts %s - secret: %d", (uint64_t)session, cnt_socket_to_string(session, connection->source), address.debug,
	        connection->secret);
}

void cnt_session_tick_rejects(cnt_session *session, cnt_connection *connection, fck_milliseconds time, fck_milliseconds delta_time)
{
	// TODO: All these patterns with early return and fetching should NEVER actually be true and cause early return. Fix and make asserts
	cnt_socket_data sock;
	cnt_address_data address;
	if (!cnt_session_fetch_data(session, connection, &sock, &address))
	{
		return;
	}

	cnt_connection_packet packet;
	SDL_zero(packet);

	cnt_connection_packet_push(&packet, CNT_CONNECTION_PACKET_TYPE_REJECT, nullptr, 0);
	cnt_sendto(sock.socket, packet.payload, packet.length, &address.address);

	SDL_Log("%llu, %s rejects %s", (uint64_t)session, cnt_socket_to_string(session, connection->source), address.debug);

	connection->last_timestamp = time;
	connection->state = CNT_CONNECTION_STATE_REJECTED;
}

void cnt_session_tick_ok(cnt_session *session, fck_item<cnt_connection_id, cnt_connection> *item, fck_milliseconds time,
                         fck_milliseconds delta_time)
{
	cnt_socket_data sock;
	cnt_address_data address;

	cnt_connection *connection = item->value;
	if (!cnt_session_fetch_data(session, connection, &sock, &address))
	{
		return;
	}

	cnt_connection_packet packet;
	SDL_zero(packet);

	cnt_connection_packet_push(&packet, CNT_CONNECTION_PACKET_TYPE_OK, nullptr, 0);
	cnt_sendto(sock.socket, packet.payload, packet.length, &address.address);

	SDL_Log("%llu, %s established with %s", (uint64_t)session, cnt_socket_to_string(session, connection->source), address.debug);

	connection->state = CNT_CONNECTION_STATE_CONNECTED;

	cnt_connection_handle handle = {*item->index};
	fck_queue_push(&session->new_connections, &handle);

	connection->last_timestamp = time;
}

static bool cnt_session_is_seq_acceptable(uint16_t seq_latest, uint16_t seq_recv)
{
	constexpr uint16_t seq_window = 0x8000;

	uint16_t difference = seq_recv - seq_latest;

	return difference < seq_window && difference > 0;
}

void cnt_session_tick_receive(cnt_session *session, fck_milliseconds time)
{
	// TODO: replace cnt_connection_packet with THE RECV BUFFER!! WE NEED MORE SPAAACE
	for (fck_item<cnt_socket_id, cnt_socket_data> item : &session->sockets)
	{
		cnt_socket sock = item.value->socket;
		cnt_address incoming_address;

		cnt_connection_recv_packet packet;
		packet.payload = session->temp_recv_buffer;
		packet.index = 0;
		packet.length = cnt_recvfrom(sock, packet.payload, cnt_socket_memory_buffer::capacity, &incoming_address);
		if (packet.length > 0)
		{
			cnt_connection_id id;
			cnt_connection *connection = cnt_session_connection_find_with_socket(session, item.index, &incoming_address, &id);
			if (connection == nullptr)
			{
				cnt_address_handle address_handle = cnt_session_address_create(session, &incoming_address);
				cnt_connection new_connection;
				SDL_zero(new_connection);
				new_connection.source = *item.index;
				new_connection.destination = address_handle.id;

				id = fck_sparse_list_add(&session->connections, &new_connection);
				connection = fck_sparse_list_view(&session->connections, id);
			}
			connection->last_timestamp = time;

			cnt_connection_packet_type type;
			void *data;
			uint16_t length;
			while (cnt_connection_packet_try_pop(&packet, &type, &data, &length))
			{
				switch (type)
				{
				case CNT_CONNECTION_PACKET_TYPE_REQUEST: {
					cnt_connection_request *request = (cnt_connection_request *)data;
					connection->secret = request->suggested_secret;
					bool is_valid_protocol = request->protocol == CNT_PROTOCOL_ID && request->version == CNT_PROTOCOL_VERSION;

					bool is_valid_endian = request->is_little_endian == cnt_connection_is_little_endian();
					CHECK_WARNING(is_valid_endian, "Endianess is different. Data flow will fail when endianess matters!")

					if (is_valid_protocol)
					{
						connection->state = CNT_CONNECTION_STATE_REQUEST_INCOMING;
					}
					else
					{
						connection->state = CNT_CONNECTION_STATE_REJECTED;
					}

					SDL_Log("%llu, %s gets request from %s", (uint64_t)session, cnt_socket_to_string(session, connection->source),
					        cnt_address_to_string(session, connection->destination));
				}
				break;
				case CNT_CONNECTION_PACKET_TYPE_ACCEPT: {
					cnt_connection_accept *accept = (cnt_connection_accept *)data;
					connection->secret = accept->directed_secret;
					connection->state = CNT_CONNECTION_STATE_ACKNOWLEDGE_CONNECTION;
					SDL_Log("%llu, %s got accepted by %s - secret: %d", (uint64_t)session,
					        cnt_socket_to_string(session, connection->source), cnt_address_to_string(session, connection->destination),
					        accept->directed_secret);
				}
				break;
				case CNT_CONNECTION_PACKET_TYPE_REJECT: {
					cnt_connection_accept *accept = (cnt_connection_accept *)data;
					fck_sparse_list_remove(&session->connections, id);
					SDL_Log("%llu, %s got rejected by %s - secret: %d", (uint64_t)session,
					        cnt_socket_to_string(session, connection->source), cnt_address_to_string(session, connection->destination),
					        accept->directed_secret);
				}
				break;
				case CNT_CONNECTION_PACKET_TYPE_OK: {
					connection->state = CNT_CONNECTION_STATE_CONNECTED;
					SDL_Log("%llu, %s established with %s", (uint64_t)session, cnt_socket_to_string(session, connection->source),
					        cnt_address_to_string(session, connection->destination));

					cnt_connection_handle handle = {id};
					fck_queue_push(&session->new_connections, &handle);
				}
				break;
				case CNT_CONNECTION_PACKET_TYPE_DATA: {
					if (connection->state != CNT_CONNECTION_STATE_CONNECTED)
					{
						break;
					}

					cnt_socket_memory_buffer *socket_memory;
					bool buffer_exists = fck_sparse_array_try_view(&session->recv_buffer, *item.index, &socket_memory);
					SDL_assert(buffer_exists);

					fck_serialiser serialiser;
					fck_serialiser_create(&serialiser, (uint8_t *)data, length);
					fck_serialiser_byte_reader(&serialiser.self);

					uint16_t seq_recv = 0;
					fck_serialise(&serialiser, &seq_recv);
					if (!cnt_session_is_seq_acceptable(connection->seq_last_recv, seq_recv))
					{
						break;
					}

					cnt_socket_memory_buffer_append(socket_memory, ((uint8_t *)data) + serialiser.at, length);

					connection->seq_last_recv = seq_recv;

					cnt_frame frame;
					frame.at = socket_memory->length - length;
					frame.generation = socket_memory->generation;
					frame.length = length;
					frame.owner = *item.index;
					frame.info.connection = {id};
					frame.info.tick = session->tick; // Maybe tick as param?
					fck_queue_push(&session->recv_frames, &frame);
				}
				break;
				}
			}
		}
	}
}

void cnt_session_tick_send(cnt_session *session, fck_milliseconds time)
{
	// Instead of using dynamic allocation, we "inject" our own stack allocated buffer
	constexpr cnt_connection_id remove_buffer_capacity = 32;
	cnt_connection_id remove_buffer[remove_buffer_capacity];
	fck_dense_list<cnt_connection_id, cnt_connection_id> remove_list;
	SDL_zero(remove_list);
	remove_list.capacity = remove_buffer_capacity;
	remove_list.data = remove_buffer;
	remove_list.count = 0;

	for (fck_item<cnt_connection_id, cnt_connection> item : &session->connections)
	{
		cnt_connection *connection = item.value;
		switch (connection->state)
		{
		case CNT_CONNECTION_STATE_NONE:
			// What to do... what to do?
			// We can time out and such at some point
			break;
		case CNT_CONNECTION_STATE_CONNECTING:
			cnt_session_tick_connecting(session, connection, time, session->tick_rate);
			break;
		case CNT_CONNECTION_STATE_REQUEST_INCOMING:
			cnt_session_tick_accepts(session, connection, time, session->tick_rate);
			break;
		case CNT_CONNECTION_STATE_REJECTED:
			cnt_session_tick_rejects(session, connection, time, session->tick_rate);
			fck_dense_list_add(&remove_list, item.index);
			break;
		case CNT_CONNECTION_STATE_ACKNOWLEDGE_CONNECTION:
			cnt_session_tick_ok(session, &item, time, session->tick_rate);
			break;
		case CNT_CONNECTION_STATE_CONNECTED:
			// ... I do not know. Maybe keep alive stuff?
			break;
		case CNT_CONNECTION_STATE_REQUEST_OUTGOING: {
			if (time - connection->last_timestamp > 5000)
			{
				cnt_session_tick_connecting(session, connection, time, session->tick_rate);
			}
			break;
		}
		case CNT_CONNECTION_STATE_WAITING_FOR_ACKNOWLDGEMENT:
			if (time - connection->last_timestamp > 5000)
			{
				cnt_session_tick_accepts(session, connection, time, session->tick_rate);
			}
			break;
		}
	}

	for (cnt_connection_id *id : &remove_list)
	{
		fck_sparse_list_remove(&session->connections, *id);
	}
}

bool cnt_session_will_tick(cnt_session *session, fck_milliseconds delta_time)
{
	fck_milliseconds next_tick_time = session->tick_time_accumulator + delta_time;
	return next_tick_time >= session->tick_rate;
}

void cnt_session_tick(cnt_session *session, fck_milliseconds time, fck_milliseconds delta_time)
{
	cnt_session_tick_receive(session, time);

	session->tick_time_accumulator = session->tick_time_accumulator + delta_time;
	while (session->tick_time_accumulator < session->tick_rate)
	{
		return;
	}

	session->tick_time_accumulator = 0;
	session->tick = session->tick + 1;

	cnt_session_tick_send(session, time);
}

void cnt_session_broadcast(cnt_session *session, void *data, size_t count)
{
	// Does the boardcast logic still make sense? Should we really do it like this?
	// Couldn't we just prepare a single buffer and hand it out multiple times (each socket)
	// Let's rethink that approach here! It is too complicated for no fucking reason. Look at send
	fck_serialiser serialiser;
	fck_serialiser_create(&serialiser, session->temp_send_buffer, cnt_socket_memory_buffer::capacity);
	fck_serialiser_byte_writer(&serialiser.self);

	cnt_connection_packet_header header;
	SDL_zero(header);
	header.type = CNT_CONNECTION_PACKET_TYPE_DATA;
	header.length = count + sizeof(header);
	fck_serialise(&serialiser, &header);

	constexpr uint16_t max_seq = 0xFFFF;

	fck_serialise(&serialiser, &session->seq);
	session->seq = (session->seq + 1) % max_seq;

	fck_serialise(&serialiser, (uint8_t *)data, count);

	for (cnt_connection *connection : &session->connections.dense)
	{
		if (connection->state != CNT_CONNECTION_STATE_CONNECTED)
		{
			continue;
		}

		cnt_socket_data sock;
		cnt_address_data address;
		if (cnt_session_fetch_data(session, connection, &sock, &address))
		{
			cnt_sendto(sock.socket, serialiser.data, serialiser.at, &address.address);
		}
	}
}

bool cnt_session_send(cnt_session *session, cnt_connection_handle *connection_handle, void *data, size_t count)
{
	cnt_connection *connection = fck_sparse_list_view(&session->connections, connection_handle->id);
	if (connection == nullptr)
	{
		return false;
	}

	if (connection->state != CNT_CONNECTION_STATE_CONNECTED)
	{
		return false;
	}

	cnt_socket_data sock;
	cnt_address_data address;
	if (!cnt_session_fetch_data(session, connection, &sock, &address))
	{
		return false;
	}

	fck_serialiser serialiser;
	fck_serialiser_create(&serialiser, session->temp_send_buffer, cnt_socket_memory_buffer::capacity);
	fck_serialiser_byte_writer(&serialiser.self);

	cnt_connection_packet_header header;
	SDL_zero(header);
	header.type = CNT_CONNECTION_PACKET_TYPE_DATA;
	header.length = count + sizeof(header);
	fck_serialise(&serialiser, &header);

	constexpr uint16_t max_seq = 0xFFFF;

	fck_serialise(&serialiser, &session->seq);
	session->seq = (session->seq + 1) % max_seq;

	fck_serialise(&serialiser, (uint8_t *)data, count);

	cnt_sendto(sock.socket, serialiser.data, serialiser.at, &address.address);

	return true;
}

bool cnt_session_try_receive_from(cnt_session *session, cnt_memory_view *view)
{
	SDL_assert(view != nullptr);

	cnt_frame *frame;
	if (!fck_queue_try_pop(&session->recv_frames, &frame))
	{
		return false;
	}

	cnt_socket_memory_buffer *socket_memory;
	bool buffer_exists = fck_sparse_array_try_view(&session->recv_buffer, frame->owner, &socket_memory);
	SDL_assert(buffer_exists);

	view->info = frame->info;
	view->data = socket_memory->data + frame->at;
	view->length = frame->length;
	return true;
}

bool cnt_session_try_dequeue_new_connection(cnt_session *session, cnt_connection_handle *handle, cnt_connection *connection)
{
	cnt_connection_handle *new_connection;
	if (fck_queue_try_pop(&session->new_connections, &new_connection))
	{
		*handle = *new_connection;
		*connection = *fck_sparse_list_view(&session->connections, new_connection->id);
		return true;
	}
	return false;
}