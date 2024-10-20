#ifndef CNT_CONNECTION_INCLUDED
#define CNT_CONNECTION_INCLUDED

#include "cnt_transport.h"
#include "ecs/fck_sparse_list.h"

using fck_millisecond = uint64_t;

using cnt_socket_id = uint8_t;
using cnt_address_id = uint8_t;
using cnt_connection_id = uint8_t;

constexpr cnt_socket_id CNT_SOCKET_INVALID_ID = ~0;
constexpr cnt_socket_id CNT_ADDRESS_INVALID_ID = ~0;
constexpr cnt_socket_id CNT_CONNECTION_INVALID_ID = ~0;

struct cnt_socket_handle
{
	cnt_socket_id id;
};

struct cnt_address_handle
{
	cnt_address_id id;
};

/*struct cnt_connection_handle
{
    cnt_connection_id id;
};*/

enum cnt_connection_state
{
	CNT_CONNECTION_STATE_REQUEST_INCOMING,
	CNT_CONNECTION_STATE_REQUEST_OUTGOING,
	CNT_CONNECTION_STATE_CONNECTING,
	CNT_CONNECTION_STATE_CONNECTED,
	CNT_CONNECTION_STATE_REJECTED,
	CNT_CONNECTION_STATE_WAITING_TO_CONNECT,
	CNT_CONNECTION_STATE_ACKNOWLEDGE_CONNECTION
};

struct cnt_connection
{
	cnt_connection_state state;
	cnt_socket_id source;
	cnt_address_id destination;
};

struct cnt_socket_data
{
	cnt_socket socket;
	cnt_address_id address;
};

struct cnt_address_data
{
	cnt_address *address;
	char debug[64];
};

struct cnt_session
{
	using system_id = uint8_t;

	fck_sparse_list<cnt_socket_id, cnt_socket_data> sockets;
	fck_sparse_list<cnt_address_id, cnt_address_data> addresses;
	fck_sparse_list<cnt_connection_id, cnt_connection> connections;

	uint64_t tick;
	fck_millisecond tick_rate;
	fck_millisecond tick_time_accumulator;
};

cnt_address_handle cnt_session_address_create(cnt_session *session, char const *ip, uint16_t port);
cnt_socket_handle cnt_session_socket_create(cnt_session *session, char const *ip, uint16_t port);
void cnt_session_connect(cnt_session *session, cnt_socket_handle const *socket, cnt_address_handle const *address);

void cnt_session_alloc(cnt_session *session, cnt_socket_id socket_capacity, cnt_address_id address_capacity,
                       cnt_connection_id connection_capacity, uint64_t tick_rate);
void cnt_session_free(cnt_session *session);
void cnt_session_tick(cnt_session *session, uint64_t time, uint64_t delta_time);

#endif // CNT_CONNECTION_INCLUDED