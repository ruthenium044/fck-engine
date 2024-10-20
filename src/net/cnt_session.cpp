#include "net/cnt_session.h"
#include "SDL3/SDL_assert.h"
#include "cnt_protocol.h"
#include "fck_checks.h"

void cnt_session_alloc(cnt_session *session, cnt_socket_id socket_capacity, cnt_address_id address_capacity,
                       cnt_connection_id connection_capacity, uint64_t tick_rate)
{
	SDL_assert(session != nullptr);
	SDL_zerop(session);

	session->tick_rate = tick_rate;

	fck_sparse_list_alloc(&session->addresses, address_capacity);
	fck_sparse_list_alloc(&session->connections, connection_capacity);
	fck_sparse_list_alloc(&session->sockets, socket_capacity);
}

void cnt_session_free(cnt_session *session)
{
	SDL_assert(session != nullptr);

	fck_sparse_list_free(&session->addresses);
	fck_sparse_list_free(&session->connections);
	fck_sparse_list_free(&session->sockets);

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

cnt_connection *cnt_session_connection_find_with_socket(cnt_session *session, cnt_socket_id const *socket_id, cnt_address *address)
{
	for (cnt_connection *connection : &session->connections.dense)
	{
		if (connection->source == *socket_id)
		{
			cnt_address *addr = fck_sparse_list_view(&session->addresses, connection->destination)->address;
			if (addr != nullptr && cnt_address_equals(addr, address))
			{
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

	return {fck_sparse_list_add(&session->sockets, &data)};
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
	CHECK_WARNING(sock != nullptr, "Cannot find socket", return false);
	CHECK_WARNING(address != nullptr, "Cannot find address", return false);

	SDL_assert(sock->socket != -1);
	SDL_assert(address->address != nullptr);

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

	cnt_connection_packet_push(&packet, CNT_CONNECTION_PACKET_TYPE_ACCEPT, nullptr, 0);
	cnt_send(sock.socket, packet.payload, packet.length, address.address);

	SDL_Log("%llu, %s accepts %s", (uint64_t)session, socket_to_string(session, connection->source), address.debug);

	connection->state = CNT_CONNECTION_STATE_WAITING_TO_CONNECT;
}

void cnt_session_tick_rejects(cnt_session *session, cnt_connection *connection, uint64_t time, uint64_t delta_time)
{
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
		packet.length = cnt_recv(sock, packet.payload, sizeof(packet.capacity), &incoming_address);

		if (packet.length > 0)
		{
			cnt_connection_packet_type type;
			void *data;
			uint8_t length;
			while (cnt_connection_packet_try_pop(&packet, &type, &data, &length))
			{
				if (type == CNT_CONNECTION_PACKET_TYPE_REQUEST)
				{
					cnt_connection *connection = cnt_session_connection_find_with_socket(session, item.index, incoming_address);
					if (connection == nullptr)
					{
						cnt_address_handle address_handle = cnt_session_address_create(session, incoming_address);
						cnt_connection connection;
						connection.source = *item.index;
						connection.destination = address_handle.id;
						connection.state = CNT_CONNECTION_STATE_REQUEST_INCOMING;
						cnt_connection_id connection_id = fck_sparse_list_add(&session->connections, &connection);
						SDL_Log("%llu, %s gets request from %s", (uint64_t)session, socket_to_string(session, connection.source),
						        address_to_string(session, connection.destination));
					}
					else
					{
						// If we are connecting ourselves, we will end up here
						connection->state = CNT_CONNECTION_STATE_REQUEST_INCOMING;
						SDL_Log("%llu, %s gets request from %s", (uint64_t)session, socket_to_string(session, connection->source),
						        address_to_string(session, connection->destination));
					}
				}

				if (type == CNT_CONNECTION_PACKET_TYPE_ACCEPT)
				{
					cnt_connection *connection = cnt_session_connection_find_with_socket(session, item.index, incoming_address);
					if (connection != nullptr)
					{
						connection->state = CNT_CONNECTION_STATE_ACKNOWLEDGE_CONNECTION;
						SDL_Log("%llu, %s got accepted by %s", (uint64_t)session, socket_to_string(session, connection->source),
						        address_to_string(session, connection->destination));
					}
				}

				if (type == CNT_CONNECTION_PACKET_TYPE_OK)
				{
					cnt_connection *connection = cnt_session_connection_find_with_socket(session, item.index, incoming_address);
					if (connection != nullptr)
					{
						connection->state = CNT_CONNECTION_STATE_CONNECTED;
						SDL_Log("%llu, %s established with %s", (uint64_t)session, socket_to_string(session, connection->source),
						        address_to_string(session, connection->destination));
					}
				}
			}
		}
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

	for (cnt_connection *connection : &session->connections.dense)
	{

		switch (connection->state)
		{
		case CNT_CONNECTION_STATE_CONNECTING:
			cnt_session_tick_connecting(session, connection, time, session->tick_rate);
			break;
		case CNT_CONNECTION_STATE_REQUEST_INCOMING:
			// TODO: Handshake secret exchange
			cnt_session_tick_accepts(session, connection, time, session->tick_rate);
			break;
		case CNT_CONNECTION_STATE_REJECTED:
			cnt_session_tick_rejects(session, connection, time, session->tick_rate);
			break;
		case CNT_CONNECTION_STATE_ACKNOWLEDGE_CONNECTION:
			cnt_session_tick_ok(session, connection, time, session->tick_rate);
			break;
		case CNT_CONNECTION_STATE_CONNECTED:
			// DATA!
			break;
		}
	}
}
