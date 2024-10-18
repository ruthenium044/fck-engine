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

cnt_connection *cnt_session_connection_find_with_socket(cnt_session *session, cnt_socket_id const *socket_id, cnt_address *address)
{
	for (cnt_connection *connection : &session->connections.dense)
	{
		if (connection->source == *socket_id)
		{
			cnt_address **addr = fck_sparse_list_view(&session->addresses, connection->destination);
			if (addr != nullptr && cnt_address_equals(*addr, address))
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
	for (fck_item<cnt_address_id, cnt_address *> item : &session->addresses)
	{
		cnt_address *address = *item.value;
		if (cnt_address_equals(addr, address))
		{
			cnt_address_free(addr);
			return {*item.index};
		}
	}
	return {fck_sparse_list_add(&session->addresses, &addr)};
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
	return {fck_sparse_list_add(&session->sockets, &sock)};
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

bool cnt_session_fetch_data(cnt_session *session, cnt_connection *connection, cnt_socket *sock, cnt_address **address)
{
	cnt_socket *sock_ptr = fck_sparse_list_view(&session->sockets, connection->source);
	cnt_address **address_ptr = fck_sparse_list_view(&session->addresses, connection->destination);
	CHECK_WARNING(sock != nullptr, "Cannot find socket", return false);
	CHECK_WARNING(address != nullptr, "Cannot find address", return false);

	SDL_assert(*sock != -1);
	SDL_assert(*address != nullptr);

	*sock = *sock_ptr;
	*address = *address_ptr;
	return true;
}

void cnt_session_tick_connecting(cnt_session *session, cnt_connection *connection, uint64_t time, uint64_t delta_time)
{
	cnt_socket sock;
	cnt_address *address;

	if (!cnt_session_fetch_data(session, connection, &sock, &address))
	{
		return;
	}

	cnt_connection_request request;
	request.base.type = CNT_CONNECTION_PACKET_TYPE_REQUEST;
	request.base.length = sizeof(request);
	request.protocol = CNT_PROTOCOL_ID;
	request.version = CNT_PROTOCOL_VERSION;
	cnt_send(sock, &request, sizeof(request), address);
	SDL_Log("%llu, %d connects to %d", (uint64_t)session, connection->source, connection->destination);

	connection->state = CNT_CONNECTION_STATE_REQUESTING;
}

void cnt_session_tick_responding(cnt_session *session, cnt_connection *connection, uint64_t time, uint64_t delta_time)
{
	cnt_socket sock;
	cnt_address *address;

	if (!cnt_session_fetch_data(session, connection, &sock, &address))
	{
		return;
	}

	cnt_connection_packet request;
	request.type = CNT_CONNECTION_PACKET_TYPE_ACCEPT;
	request.length = sizeof(request);
	cnt_send(sock, &request, sizeof(request), address);
	SDL_Log("%llu, %d accepts %d", (uint64_t)session, connection->source, connection->destination);
	connection->state = CNT_CONNECTION_STATE_CONNECTED;
}

void cnt_session_tick_receive(cnt_session *session, uint64_t time, uint64_t delta_time)
{
	uint8_t buffer[1024];
	for (fck_item<cnt_socket_id, cnt_socket> item : &session->sockets)
	{
		cnt_socket *sock = item.value;
		cnt_address *incoming_address;
		size_t bytes = cnt_recv(*sock, buffer, sizeof(buffer), &incoming_address);
		if (bytes > 0)
		{
			cnt_connection_packet *header = (cnt_connection_packet *)buffer;
			if (header->type == CNT_CONNECTION_PACKET_TYPE_REQUEST)
			{
				cnt_connection *connection = cnt_session_connection_find_with_socket(session, item.index, incoming_address);
				if (connection == nullptr)
				{
					cnt_address_handle address_handle = cnt_session_address_create(session, incoming_address);
					cnt_connection connection;
					connection.source = *item.index;
					connection.destination = address_handle.id;
					connection.state = CNT_CONNECTION_STATE_REQUESTED;
					cnt_connection_id connection_id = fck_sparse_list_add(&session->connections, &connection);
					SDL_Log("%llu, %d gets request from %d", (uint64_t)session, connection.source, connection.destination);
				}
				else
				{
					connection->state = CNT_CONNECTION_STATE_REQUESTED;
					SDL_Log("%llu, %d gets request from %d", (uint64_t)session, connection->source, connection->destination);
				}
			}
			if (header->type == CNT_CONNECTION_PACKET_TYPE_ACCEPT)
			{
				cnt_connection *connection = cnt_session_connection_find_with_socket(session, item.index, incoming_address);
				if (connection != nullptr)
				{
					connection->state = CNT_CONNECTION_STATE_CONNECTED;
					SDL_Log("%llu, %d established with %d", (uint64_t)session, connection->source, connection->destination);
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
		case CNT_CONNECTION_STATE_REQUESTED:
			cnt_session_tick_responding(session, connection, time, session->tick_rate);
			// DATA!
			break;
		case CNT_CONNECTION_STATE_CONNECTING:
			cnt_session_tick_connecting(session, connection, time, session->tick_rate);
			break;
		case CNT_CONNECTION_STATE_CONNECTED:
			// DATA!
			break;
		}
	}
}
