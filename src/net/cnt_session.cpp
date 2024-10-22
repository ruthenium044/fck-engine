#include "net/cnt_session.h"
#include "SDL3/SDL_assert.h"
#include "cnt_protocol.h"
#include "fck_checks.h"

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

void cnt_socket_memory_buffer_prepare_send(cnt_socket_memory_buffer *socket_memory)
{
	cnt_socket_memory_buffer_clear(socket_memory);

	// Emplace a header at the start to propagate package info!
	cnt_connection_packet_header header;
	SDL_zero(header);
	header.type = CNT_CONNECTION_PACKET_TYPE_DATA;
	header.length = 0;
	cnt_socket_memory_buffer_append(socket_memory, (uint8_t *)&header, sizeof(header));
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

	fck_sparse_array_alloc(&session->send_buffer, socket_capacity);
	fck_sparse_array_alloc(&session->recv_buffer, socket_capacity);
	fck_queue_alloc(&session->recv_frames, cnt_session::maximum_frame_capacity);
}

void cnt_session_free(cnt_session *session)
{
	SDL_assert(session != nullptr);

	for (cnt_socket_memory_buffer *memory : &session->send_buffer.dense)
	{
		cnt_socket_memory_buffer_free(memory);
	}

	for (cnt_socket_memory_buffer *memory : &session->recv_buffer.dense)
	{
		cnt_socket_memory_buffer_free(memory);
	}

	fck_sparse_list_free(&session->addresses);
	fck_sparse_list_free(&session->connections);
	fck_sparse_list_free(&session->sockets);

	fck_sparse_array_free(&session->send_buffer);
	fck_sparse_array_free(&session->recv_buffer);

	fck_queue_free(&session->recv_frames);

	SDL_zerop(session);
}

static const char *address_to_string(cnt_session *session, cnt_address_id id)
{
	return fck_sparse_list_view(&session->addresses, id)->debug;
}

static const char *socket_to_string(cnt_session *session, cnt_socket_id id)
{
	cnt_address_id address_id = fck_sparse_list_view(&session->sockets, id)->address;
	return fck_sparse_list_view(&session->addresses, address_id)->debug;
}

cnt_connection *cnt_session_connection_find_with_socket(cnt_session *session, cnt_socket_id const *socket_id, cnt_address *address,
                                                        cnt_connection_id *connection_id = nullptr)
{
	for (fck_item<cnt_connection_id, cnt_connection> item : &session->connections)
	{
		cnt_connection *connection = item.value;
		if (connection->source == *socket_id)
		{
			cnt_address *addr = fck_sparse_list_view(&session->addresses, connection->destination)->address;
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
		cnt_address *address = item.value->address;
		if (cnt_address_equals(addr, address))
		{
			cnt_address_free(addr);
			return {*item.index};
		}
	}

	cnt_address_data data;
	data.address = addr;

	cnt_address_as_string(addr, data.debug, sizeof(data.debug));

	return {fck_sparse_list_add(&session->addresses, &data)};
}

cnt_address_handle cnt_session_address_create(cnt_session *session, char const *ip, uint16_t port)
{
	cnt_address *addr = cnt_address_from_string(ip, port);
	return cnt_session_address_create(session, addr);
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

	// Initial preparation for send buffer
	cnt_socket_memory_buffer_prepare_send(&socket_send_memory);

	fck_sparse_array_emplace(&session->send_buffer, socket_id, &socket_send_memory);
	fck_sparse_array_emplace(&session->recv_buffer, socket_id, &socket_recv_memory);

	return {socket_id};
}

void cnt_session_connect(cnt_session *session, cnt_socket_handle const *socket, cnt_address_handle const *address)
{
	cnt_connection connection;
	connection.source = socket->id;
	connection.destination = address->id;

	for (fck_item<cnt_address_id, cnt_connection> item : &session->connections)
	{
		cnt_connection *current = item.value;
		if (current->destination == connection.destination && current->source && connection.source)
		{
			return;
		}
	}
	connection.state = CNT_CONNECTION_STATE_CONNECTING;
	cnt_connection_id connection_id = fck_sparse_list_add(&session->connections, &connection);
}

bool cnt_session_fetch_data(cnt_session *session, cnt_connection *connection, cnt_socket_data *sock, cnt_address_data *address)
{
	cnt_socket_data *sock_ptr = fck_sparse_list_view(&session->sockets, connection->source);
	cnt_address_data *address_ptr = fck_sparse_list_view(&session->addresses, connection->destination);
	CHECK_WARNING(sock_ptr != nullptr, "Cannot find socket", return false);
	CHECK_WARNING(address_ptr != nullptr, "Cannot find address", return false);

	SDL_assert(sock_ptr->socket != -1);
	SDL_assert(address_ptr->address != nullptr);

	*sock = *sock_ptr;
	*address = *address_ptr;
	return true;
}

void cnt_session_tick_connecting(cnt_session *session, cnt_connection *connection, uint64_t time, uint64_t delta_time)
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
	request.suggested_secret = 0;

	cnt_connection_packet_push(&packet, CNT_CONNECTION_PACKET_TYPE_REQUEST, &request, sizeof(request));

	cnt_send(sock.socket, packet.payload, packet.length, address.address);
	SDL_Log("%llu, %s connects to %s", (uint64_t)session, socket_to_string(session, connection->source), address.debug);

	connection->state = CNT_CONNECTION_STATE_REQUEST_OUTGOING;
}

void cnt_session_tick_accepts(cnt_session *session, cnt_connection *connection, uint64_t time, uint64_t delta_time)
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

	uint64_t secret = time | (time >> 31);
	accept.directed_secret = secret;
	cnt_connection_packet_push(&packet, CNT_CONNECTION_PACKET_TYPE_ACCEPT, &accept, sizeof(accept));
	cnt_send(sock.socket, packet.payload, packet.length, address.address);

	connection->secret = accept.directed_secret;
	connection->state = CNT_CONNECTION_STATE_WAITING_FOR_ACKNOWLDGEMENT;

	SDL_Log("%llu, %s accepts %s - secret: %d", (uint64_t)session, socket_to_string(session, connection->source), address.debug,
	        connection->secret);
}

void cnt_session_tick_rejects(cnt_session *session, cnt_connection *connection, uint64_t time, uint64_t delta_time)
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
	cnt_send(sock.socket, packet.payload, packet.length, address.address);

	SDL_Log("%llu, %s rejects %s", (uint64_t)session, socket_to_string(session, connection->source), address.debug);

	connection->state = CNT_CONNECTION_STATE_REJECTED;
}

void cnt_session_tick_data_send(cnt_session *session, cnt_connection *connection, uint64_t time, uint64_t delta_time)
{
	cnt_socket_data sock;
	cnt_address_data address;
	if (!cnt_session_fetch_data(session, connection, &sock, &address))
	{
		return;
	}
	cnt_socket_memory_buffer *socket_memory;
	bool buffer_exists = fck_sparse_array_try_view(&session->send_buffer, connection->source, &socket_memory);
	SDL_assert(buffer_exists);

	// Edit the header with the correct length to read the data correctly
	cnt_connection_packet_header *header = (cnt_connection_packet_header *)socket_memory->data;
	header->length = socket_memory->length - sizeof(*header);
	cnt_send(sock.socket, socket_memory->data, socket_memory->length, address.address);
}

void cnt_session_tick_ok(cnt_session *session, cnt_connection *connection, uint64_t time, uint64_t delta_time)
{
	cnt_socket_data sock;
	cnt_address_data address;

	if (!cnt_session_fetch_data(session, connection, &sock, &address))
	{
		return;
	}

	cnt_connection_packet packet;
	SDL_zero(packet);

	cnt_connection_packet_push(&packet, CNT_CONNECTION_PACKET_TYPE_OK, nullptr, 0);
	cnt_send(sock.socket, packet.payload, packet.length, address.address);

	SDL_Log("%llu, %s established with %s", (uint64_t)session, socket_to_string(session, connection->source), address.debug);

	connection->state = CNT_CONNECTION_STATE_CONNECTED;
}

void cnt_session_tick_receive(cnt_session *session, uint64_t time, uint64_t delta_time)
{
	cnt_connection_packet packet;
	for (fck_item<cnt_socket_id, cnt_socket_data> item : &session->sockets)
	{
		cnt_socket sock = item.value->socket;
		cnt_address *incoming_address;
		packet.length = cnt_recv(sock, packet.payload, packet.capacity, &incoming_address);

		if (packet.length > 0)
		{
			cnt_connection_packet_type type;
			void *data;
			uint8_t length;
			while (cnt_connection_packet_try_pop(&packet, &type, &data, &length))
			{
				switch (type)
				{

				case CNT_CONNECTION_PACKET_TYPE_REQUEST: {
					cnt_connection *connection = cnt_session_connection_find_with_socket(session, item.index, incoming_address);
					if (connection == nullptr)
					{
						cnt_connection_request *request = (cnt_connection_request *)data;
						cnt_address_handle address_handle = cnt_session_address_create(session, incoming_address);
						cnt_connection connection;
						connection.source = *item.index;
						connection.destination = address_handle.id;
						connection.secret = request->suggested_secret;
						bool is_valid_protocol = request->protocol == CNT_PROTOCOL_ID && request->version == CNT_PROTOCOL_VERSION;
						SDL_Log("%llu, %s gets request from %s", (uint64_t)session, socket_to_string(session, connection.source),
						        address_to_string(session, connection.destination));
						if (is_valid_protocol)
						{
							connection.state = CNT_CONNECTION_STATE_REQUEST_INCOMING;
						}
						else
						{
							connection.state = CNT_CONNECTION_STATE_REJECTED;
						}
						cnt_connection_id connection_id = fck_sparse_list_add(&session->connections, &connection);
					}
					else
					{
						// If we are connecting ourselves, we will end up here
						connection->state = CNT_CONNECTION_STATE_REQUEST_INCOMING;
						SDL_Log("%llu, %s gets request from %s", (uint64_t)session, socket_to_string(session, connection->source),
						        address_to_string(session, connection->destination));
					}
				}
				break;
				case CNT_CONNECTION_PACKET_TYPE_ACCEPT: {
					cnt_connection_accept *accept = (cnt_connection_accept *)data;
					cnt_connection *connection = cnt_session_connection_find_with_socket(session, item.index, incoming_address);
					if (connection != nullptr)
					{
						connection->secret = accept->directed_secret;
						connection->state = CNT_CONNECTION_STATE_ACKNOWLEDGE_CONNECTION;
						SDL_Log("%llu, %s got accepted by %s - secret: %d", (uint64_t)session,
						        socket_to_string(session, connection->source), address_to_string(session, connection->destination),
						        accept->directed_secret);
					}
				}
				break;
				case CNT_CONNECTION_PACKET_TYPE_REJECT: {
					cnt_connection_accept *accept = (cnt_connection_accept *)data;
					cnt_connection_id id;
					cnt_connection *connection = cnt_session_connection_find_with_socket(session, item.index, incoming_address, &id);
					if (connection != nullptr)
					{
						// Rejected :(
						fck_sparse_list_remove(&session->connections, id);
						SDL_Log("%llu, %s got rejected by %s - secret: %d", (uint64_t)session,
						        socket_to_string(session, connection->source), address_to_string(session, connection->destination),
						        accept->directed_secret);
					}
				}
				break;
				case CNT_CONNECTION_PACKET_TYPE_OK: {
					cnt_connection *connection = cnt_session_connection_find_with_socket(session, item.index, incoming_address);
					if (connection != nullptr)
					{
						connection->state = CNT_CONNECTION_STATE_CONNECTED;
						SDL_Log("%llu, %s established with %s", (uint64_t)session, socket_to_string(session, connection->source),
						        address_to_string(session, connection->destination));
					}
				}
				break;
				case CNT_CONNECTION_PACKET_TYPE_DATA: {
					cnt_connection_id id;
					cnt_connection *connection = cnt_session_connection_find_with_socket(session, item.index, incoming_address, &id);
					if (connection != nullptr)
					{
						cnt_socket_memory_buffer *socket_memory;
						bool buffer_exists = fck_sparse_array_try_view(&session->recv_buffer, *item.index, &socket_memory);
						SDL_assert(buffer_exists);
						cnt_socket_memory_buffer_append(socket_memory, (uint8_t *)data, length);

						cnt_frame frame;
						frame.at = socket_memory->length - length;
						frame.generation = socket_memory->generation;
						frame.length = length;
						frame.owner = *item.index;
						frame.info.connection = id;
						frame.info.tick = session->tick; // Maybe tick as param?
						fck_queue_push(&session->recv_frames, &frame);
					}
				}
				break;
				}
			}
		}
	}
}

void cnt_session_tick_send(cnt_session *session, uint64_t time, uint64_t delta_time)
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
			cnt_session_tick_ok(session, connection, time, session->tick_rate);
			break;
		case CNT_CONNECTION_STATE_CONNECTED:
			cnt_session_tick_data_send(session, connection, time, session->tick_rate);
			break;
		case CNT_CONNECTION_STATE_REQUEST_OUTGOING:
			// Retry a few times if it takes too long
			break;
		case CNT_CONNECTION_STATE_WAITING_FOR_ACKNOWLDGEMENT:
			// Retry a few times if it takes too long
			break;
		}
	}

	for (cnt_connection_id *id : &remove_list)
	{
		fck_sparse_list_remove(&session->connections, *id);
	}

	// Reset the buffers
	for (cnt_socket_memory_buffer *socket_memory : &session->send_buffer.dense)
	{
		cnt_socket_memory_buffer_prepare_send(socket_memory);
	}
}

void cnt_session_tick(cnt_session *session, uint64_t time, uint64_t delta_time)
{
	cnt_session_tick_receive(session, time, delta_time);

	session->tick_time_accumulator = session->tick_time_accumulator + delta_time;
	while (session->tick_time_accumulator < session->tick_rate)
	{
		return;
	}

	session->tick_time_accumulator = 0;
	session->tick = session->tick + 1;

	cnt_session_tick_send(session, time, delta_time);
}

void cnt_session_send_to_all(cnt_session *session, void *data, size_t count)
{
	for (fck_item<cnt_socket_id, cnt_socket_data> item : &session->sockets)
	{
		cnt_socket_memory_buffer *socket_memory;
		bool socket_memory_exists = fck_sparse_array_try_view(&session->send_buffer, *item.index, &socket_memory);
		SDL_assert(socket_memory_exists && "Socket memory is associative and cannot exist without a socket");

		cnt_socket_memory_buffer_append(socket_memory, (uint8_t *)data, count);
	}
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