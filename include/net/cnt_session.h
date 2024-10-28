#ifndef CNT_CONNECTION_INCLUDED
#define CNT_CONNECTION_INCLUDED

// TODO: Get rid of this header and move it to src
#include "cnt_transport.h"
#include "ecs/fck_queue.h"
#include "ecs/fck_sparse_array.h"
#include "ecs/fck_sparse_list.h"

using fck_milliseconds = uint64_t;

using cnt_socket_id = uint8_t;
using cnt_address_id = uint8_t;
using cnt_connection_id = uint8_t;

constexpr cnt_socket_id CNT_SOCKET_INVALID_ID = ~0;
constexpr cnt_address_id CNT_ADDRESS_INVALID_ID = ~0;
constexpr cnt_connection_id CNT_CONNECTION_INVALID_ID = ~0;

struct cnt_socket_handle
{
	cnt_socket_id id;
};

struct cnt_address_handle
{
	cnt_address_id id;
};

struct cnt_connection_handle
{
	cnt_connection_id id;
};

enum cnt_connection_state
{
	CNT_CONNECTION_STATE_REQUEST_INCOMING,
	CNT_CONNECTION_STATE_REQUEST_OUTGOING,
	CNT_CONNECTION_STATE_CONNECTING,
	CNT_CONNECTION_STATE_CONNECTED,
	CNT_CONNECTION_STATE_REJECTED,
	CNT_CONNECTION_STATE_WAITING_FOR_ACKNOWLDGEMENT,
	CNT_CONNECTION_STATE_ACKNOWLEDGE_CONNECTION
};

enum cnt_connection_flag
{
	CNT_CONNECTION_FLAG_NONE,
	CNT_CONNECTION_FLAG_INITIATOR = 1 << 0,
};

struct cnt_connection
{
	cnt_connection_state state;
	cnt_socket_id source;
	cnt_address_id destination;

	fck_milliseconds last_timestamp;
	uint32_t secret;

	cnt_connection_flag flags;
};

struct cnt_socket_data
{
	cnt_socket socket;
	cnt_address_id address;
};

struct cnt_address_data
{
	cnt_address address;
	char debug[64];
};

struct cnt_frame_info
{
	uint64_t tick;
	// ... Maybe time/
	cnt_connection_id connection;
};

struct cnt_frame
{
	cnt_frame_info info;
	cnt_socket_id owner;
	uint64_t generation;

	uint16_t at;
	uint16_t length;
};

struct cnt_socket_memory_buffer
{
	// Maybe using the dense list here is a bit shit
	static constexpr uint16_t capacity = ~0;
	uint8_t *data;
	uint16_t length;

	uint64_t generation;
};

struct cnt_memory_view
{
	cnt_frame_info info;

	uint8_t *data;
	uint16_t length;
};

struct cnt_session
{
	static constexpr size_t maximum_frame_capacity = 1024;

	fck_sparse_list<cnt_socket_id, cnt_socket_data> sockets;
	fck_sparse_list<cnt_address_id, cnt_address_data> addresses;
	fck_sparse_list<cnt_connection_id, cnt_connection> connections;

	fck_queue<cnt_connection_id, cnt_connection_handle> new_connections;

	// About 64kb
	fck_sparse_array<cnt_socket_id, cnt_socket_memory_buffer> send_buffer;
	fck_sparse_array<cnt_socket_id, cnt_socket_memory_buffer> recv_buffer;
	fck_queue<uint16_t, cnt_frame> recv_frames;

	uint64_t tick;
	fck_milliseconds tick_rate;
	fck_milliseconds tick_time_accumulator;
};

cnt_address_handle cnt_session_address_create(cnt_session *session, char const *ip, uint16_t port);
cnt_socket_handle cnt_session_socket_create(cnt_session *session, char const *ip, uint16_t port);
void cnt_session_connect(cnt_session *session, cnt_socket_handle const *socket, cnt_address_handle const *address);

const char *cnt_address_to_string(cnt_session *session, cnt_address_id id);
const char *cnt_socket_to_string(cnt_session *session, cnt_socket_id id);

// TODO: Disconnect
void cnt_session_alloc(cnt_session *session, cnt_socket_id socket_capacity, cnt_address_id address_capacity,
                       cnt_connection_id connection_capacity, uint64_t tick_rate);
void cnt_session_free(cnt_session *session);
bool cnt_session_will_tick(cnt_session *session, fck_milliseconds delta_time);
void cnt_session_tick(cnt_session *session, fck_milliseconds time, fck_milliseconds delta_time);

bool cnt_session_try_dequeue_new_connection(cnt_session *session, cnt_connection *connection);
void cnt_session_send_to_all(cnt_session *session, void *data, size_t count);
bool cnt_session_try_receive_from(cnt_session *session, cnt_memory_view *view);

#endif // CNT_CONNECTION_INCLUDED