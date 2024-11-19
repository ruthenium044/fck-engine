#ifndef CNT_NET_TYPES_INCLUDED
#define CNT_NET_TYPES_INCLUDED

#include <SDL3/SDL_stdinc.h>

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

constexpr static cnt_connection_handle CNT_CONNECTION_HANDLE_INVALID = {CNT_CONNECTION_INVALID_ID};

enum cnt_connection_state
{
	CNT_CONNECTION_STATE_NONE,
	CNT_CONNECTION_STATE_REQUEST_INCOMING,
	CNT_CONNECTION_STATE_REQUEST_OUTGOING,
	CNT_CONNECTION_STATE_CONNECTING,
	CNT_CONNECTION_STATE_CONNECTED,
	CNT_CONNECTION_STATE_REJECTED,
	CNT_CONNECTION_STATE_WAITING_FOR_ACKNOWLDGEMENT,
	CNT_CONNECTION_STATE_ACKNOWLEDGE_CONNECTION
};

enum cnt_connection_flag : uint8_t
{
	CNT_CONNECTION_FLAG_NONE,
	CNT_CONNECTION_FLAG_CLIENT = 1 << 0,
};

#endif // !CNT_NET_TYPES_INCLUDED
