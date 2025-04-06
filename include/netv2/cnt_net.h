#ifndef CNT_NET_INCLUDED
#define CNT_NET_INCLUDED

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_atomic.h>
#include <SDL3/SDL_stdinc.h>

#define CNT_ANY_IP "0.0.0.0"
#define CNT_ANY_PORT 0

struct cnt_sparse_index
{
	uint32_t index;
};

struct cnt_dense_index
{
	uint32_t index;
};

struct cnt_sparse_list
{
	// This shit is still kind of confusing
	// sparse elements POINT to a dense array element
	// dense elements POINT to a sparse array element
	// TODO: Introduce something like a cnt_access_index for the user since sparse index is doing two jobs now
	cnt_sparse_index *sparse;
	cnt_dense_index *dense;

	uint32_t control_bit_mask;
	uint32_t free_head;

	uint32_t capacity;
	uint32_t count;
};

struct cnt_sock
{
	uint8_t handle[8];
};

struct cnt_ip
{
	uint8_t address[32];
};

struct cnt_message_64_bytes
{
	cnt_ip ip;

	uint8_t payload_count;
	uint8_t payload[64];
};

// It is actually a stack, but fuck it, we want to send the newest messages AS FAST AS POSSIBLE
// We might drop messages anyway
struct cnt_message_64_bytes_queue
{
	cnt_message_64_bytes *messages;

	uint32_t count;
	uint32_t capacity;
};

struct cnt_stream
{
	// We make these int since LZ4 prefers it... I guess
	int at;
	int capacity;

	uint8_t *data;
};

struct cnt_transport
{
	cnt_stream stream;
};

struct cnt_compression
{
	cnt_stream stream;
};

struct cnt_ip_container
{
	cnt_ip *addresses;

	uint32_t capacity;
	uint32_t count;
};

enum cnt_protocol_state_common : uint8_t
{
	// At NO time the protocol shall be 0, this flag only exists for debugging
	CNT_PROTOCOL_STATE_COMMON_NONE = 0,
	CNT_PROTOCOL_STATE_COMMON_OK = 4,
	CNT_PROTOCOL_STATE_COMMON_DISCONNECT = 6,
	CNT_PROTOCOL_STATE_COMMON_KICKED = 7,
};

enum cnt_protocol_state_client : uint8_t
{
	CNT_PROTOCOL_STATE_CLIENT_NONE = CNT_PROTOCOL_STATE_COMMON_NONE,
	CNT_PROTOCOL_STATE_CLIENT_REQUEST = 1,
	CNT_PROTOCOL_STATE_CLIENT_ANSWER = 3,
	CNT_PROTOCOL_STATE_CLIENT_OK = CNT_PROTOCOL_STATE_COMMON_OK,
	CNT_PROTOCOL_STATE_CLIENT_DISCONNECT = CNT_PROTOCOL_STATE_COMMON_DISCONNECT,
	CNT_PROTOCOL_STATE_CLIENT_KICKED = CNT_PROTOCOL_STATE_COMMON_KICKED,
};

enum cnt_protocol_state_host : uint8_t
{
	CNT_PROTOCOL_STATE_HOST_NONE = CNT_PROTOCOL_STATE_COMMON_NONE,
	CNT_PROTOCOL_STATE_HOST_CHALLENGE = 2,
	CNT_PROTOCOL_STATE_HOST_OK = CNT_PROTOCOL_STATE_COMMON_OK,
	CNT_PROTOCOL_STATE_HOST_RESOLUTION_REJECT = 5, // Everything bigger than 4 is out of the handshake - in other words, rejected
	CNT_PROTOCOL_STATE_HOST_DISCONNECT = CNT_PROTOCOL_STATE_COMMON_DISCONNECT,
	CNT_PROTOCOL_STATE_HOST_KICKED = CNT_PROTOCOL_STATE_COMMON_KICKED,
};

struct cnt_protocol_client
{
	uint32_t prefix;
	cnt_protocol_state_client state;

	// Client explicitly states its id for the server
	// The server reads id, compares secret, maybe compares ip
	// They work together to make resource resolution faster
	cnt_sparse_index id;

	uint8_t extra_payload_count;
	uint8_t extra_payload[16];
};

struct cnt_protocol_host
{
	uint32_t prefix;
	cnt_protocol_state_host state;

	// The host propagates the client's id implicitly within the payload at some point
	// during the handshake. The client needs to remember it. It could and should become part of the challenge
	uint8_t extra_payload_count;
	uint8_t extra_payload[16];
};

struct cnt_connection
{
	cnt_sock src;
	cnt_ip dst;
};

enum cnt_secret_state : uint8_t
{
	CNT_SECRET_STATE_EMPTY,
	CNT_SECRET_STATE_OUTDATED,
	CNT_SECRET_STATE_ACCEPTED,
	CNT_SECRET_STATE_REJECTED
};

struct cnt_secret
{
	uint16_t public_value;
	uint16_t private_value;
	cnt_secret_state state;
};

struct cnt_star
{
	cnt_sock sock;
	cnt_ip_container destinations;
};

struct cnt_client
{
	cnt_sparse_index id_on_host;
	cnt_connection connection;
	cnt_secret secret;

	cnt_protocol_state_client protocol;
	cnt_protocol_state_host host_protocol;

	uint8_t attempts;
	// Disconnect
	// uint64_t timestamp;
};

struct cnt_client_on_host
{
	cnt_sparse_index id;
	cnt_secret secret;

	cnt_protocol_state_host protocol;
	uint8_t attempts;
	// Disconnect
	// uint64_t timestamp;
};

struct cnt_client_on_host_kicked
{
	uint64_t timestamp;
	cnt_sparse_index id;
};

struct cnt_host
{
	uint64_t recv_count;
	uint64_t send_count;

	cnt_sparse_list mapping;
	cnt_ip *ip_lookup;
	cnt_client_on_host *client_states;
};

struct cnt_user_client_frame
{
	uint8_t command_code;
	uint32_t count;
	uint8_t data[1];
};

struct cnt_queue_header
{
	uint32_t head;
	uint32_t tail;
	uint32_t capacity;
};

struct cnt_user_client_frame_queue
{
	cnt_queue_header header;

	// Needs to be last
	cnt_user_client_frame *frames[1];
};

struct cnt_user_client_frame_spsc_queue
{
	// Can be nullptr, &queues[0] or &queues[1]
	// Shared
	cnt_user_client_frame_queue *active;

	// Internal
	cnt_user_client_frame_queue *queues[2];
	uint8_t current_inactive;
};

struct cnt_user_host_frame
{
	uint32_t count;
	cnt_sparse_index client_id;

	uint8_t data[1];
};

struct cnt_user_host_frame_queue
{
	cnt_queue_header header;

	// Needs to be last
	cnt_user_host_frame *frames[1];
};

struct cnt_user_host_frame_spsc_queue
{
	// Can be nullptr, &queues[0] or &queues[1]
	// Shared
	cnt_user_host_frame_queue *active;

	// Internal
	cnt_user_host_frame_queue *queues[2];
	uint8_t current_inactive;
};

enum cnt_user_client_command_type
{
	CNT_USER_CLIENT_COMMAND_TYPE_QUIT,
	CNT_USER_CLIENT_COMMAND_TYPE_RESTART
};

struct cnt_user_client_command
{
	cnt_user_client_command_type type;
};

struct cnt_user_client_command_queue
{
	cnt_queue_header header;

	cnt_user_client_command commands[1];
};

struct cnt_user_client_command_spsc_queue
{
	cnt_user_client_command_queue *active;
	cnt_user_client_command_queue *queues[2];
	uint8_t current_inactive;
};

enum cnt_user_host_command_type
{
	CNT_USER_HOST_COMMAND_TYPE_QUIT,
	CNT_USER_HOST_COMMAND_TYPE_RESTART,
	CNT_USER_HOST_COMMAND_TYPE_KICK
};

struct cnt_user_host_command_common
{
	cnt_user_client_command_type type;
};

struct cnt_user_host_command_kick
{
	cnt_user_client_command_type type;
	cnt_sparse_index client;
};

union cnt_user_host_command {
	cnt_user_host_command_type type;
	cnt_user_host_command_common common;
	cnt_user_host_command_kick kick;
};

struct cnt_user_host_command_queue
{
	cnt_queue_header header;

	cnt_user_host_command commands[1];
};

struct cnt_user_host_command_spsc_queue
{
	cnt_user_host_command_queue *active;
	cnt_user_host_command_queue *queues[2];
	uint8_t current_inactive;
};

struct cnt_user_frequency
{
	SDL_AtomicU32 ms;
};

struct cnt_net_engine_state
{
	SDL_AtomicU32 state;
};

struct cnt_client_state
{
	SDL_AtomicU32 state;
};

struct cnt_host_state
{
	SDL_AtomicU32 state;
};

enum cnt_net_engine_state_type
{
	CNT_NET_ENGINE_STATE_TYPE_NONE = 0, // IDK what to do in this case, tbh. I also do not really care
	CNT_NET_ENGINE_STATE_TYPE_OPENING = 1,
	CNT_NET_ENGINE_STATE_TYPE_RUNNING = 2,
	CNT_NET_ENGINE_STATE_TYPE_SHUTTING_DOWN = 3,
	CNT_NET_ENGINE_STATE_TYPE_CLOSED = 4
};

struct cnt_user_client
{
	cnt_user_client_frame_spsc_queue send_queue;
	cnt_user_client_frame_spsc_queue recv_queue;

	cnt_user_client_command_spsc_queue command_queue;

	cnt_user_frequency frequency;

	cnt_net_engine_state net_engine_state;

	cnt_host_state state_on_host;
	cnt_client_state protocol_state;

	const char *host_ip;
	uint16_t host_port;
};

struct cnt_user_host
{
	cnt_user_host_frame_spsc_queue send_queue;
	cnt_user_host_frame_spsc_queue recv_queue;

	cnt_user_host_command_spsc_queue command_queue;

	cnt_user_frequency frequency;

	cnt_net_engine_state net_engine_state;

	const char *host_ip;
	uint16_t host_port;
};

// Client on host mapping - yeehaw
void cnt_sparse_list_open(cnt_sparse_list *mapping, uint32_t capacity);
void cnt_sparse_list_close(cnt_sparse_list *mapping);

// Address
cnt_ip *cnt_ip_create(cnt_ip *address, const char *ip, uint16_t port);

// Socket
cnt_sock *cnt_sock_open(cnt_sock *sock, const char *ip, uint16_t port);
void cnt_sock_close(cnt_sock *sock);

// Messages
cnt_message_64_bytes_queue *cnt_message_queue_64_bytes_open(cnt_message_64_bytes_queue *queue, uint32_t capacity);
cnt_message_64_bytes *cnt_message_queue_64_bytes_push(cnt_message_64_bytes_queue *queue, cnt_message_64_bytes const *message);
void cnt_message_queue_64_bytes_clear(cnt_message_64_bytes_queue *queue);
void cnt_message_queue_64_bytes_close(cnt_message_64_bytes_queue *queue);

// Stream
cnt_stream *cnt_stream_create(cnt_stream *stream, uint8_t *data, int count);
cnt_stream *cnt_stream_open(cnt_stream *stream, int capacity);
void cnt_stream_close(cnt_stream *stream);

// Transport - Maybe call it payload?
cnt_transport *cnt_transport_open(cnt_transport *transport, int capacity);
// Maybe interact with the transport.stream layer directly?
// cnt_transport *cnt_transport_write(cnt_transport *transport, cnt_stream *stream);
// cnt_transport *cnt_transport_read(cnt_transport *transport, cnt_stream *stream);
void cnt_transport_close(cnt_transport *transport);

// Compression
cnt_compression *cnt_compression_open(cnt_compression *compression, uint32_t capacity);
cnt_compression *cnt_compress(cnt_compression *compression, cnt_stream *stream);
cnt_stream *cnt_decompress(cnt_compression *compression, cnt_stream *stream);
void cnt_compression_close(cnt_compression *compression);

// Connection
cnt_connection *cnt_connection_open(cnt_connection *connection, cnt_sock *src, cnt_ip *dst);
cnt_connection *cnt_connection_send(cnt_connection *connection, cnt_stream *stream);
cnt_connection *cnt_connection_recv(cnt_connection *connection, cnt_stream *stream);
void cnt_connection_close(cnt_connection *connection);

// Star
cnt_star *cnt_star_open(cnt_star *star, cnt_sock *src, uint32_t max_connections);
cnt_star *cnt_star_send(cnt_star *star, cnt_stream *stream);
cnt_ip *cnt_star_recv(cnt_star *star, cnt_ip *address, cnt_stream *stream);
cnt_star *cnt_star_add(cnt_star *star, cnt_ip *addr);
cnt_ip *cnt_star_remove(cnt_star *star, cnt_ip *addr);
void cnt_star_close(cnt_star *star);

// Client
cnt_client *cnt_client_open(cnt_client *client, cnt_connection *connection);
cnt_client *cnt_client_send(cnt_client *client, cnt_stream *stream);
cnt_client *cnt_client_recv(cnt_client *client, cnt_stream *stream);
void cnt_client_close(cnt_client *client);

// Host
cnt_host *cnt_host_open(cnt_host *host, uint32_t max_connections);
cnt_host *cnt_host_send(cnt_host *host, cnt_stream *stream, cnt_ip_container *container, cnt_message_64_bytes_queue *messages);
cnt_client_on_host *cnt_host_recv(cnt_host *host, cnt_ip *client_addr, cnt_stream *stream);
void cnt_host_close(cnt_host *host);

// User Realm
// Utility
const char *cnt_net_engine_state_type_to_string(cnt_net_engine_state_type state);
const char* cnt_protocol_state_host_to_string(cnt_protocol_state_host state);
const char* cnt_protocol_state_client_to_string(cnt_protocol_state_client state);

// Client
cnt_user_client *cnt_user_client_open(cnt_user_client *user, const char *host_ip, uint16_t port, uint32_t frequency);
cnt_user_client *cnt_user_client_shut_down(cnt_user_client *user);
cnt_net_engine_state_type cnt_user_client_get_state(cnt_user_client *user);
cnt_protocol_state_client cnt_user_client_get_client_protocol_state(cnt_user_client* user);
cnt_protocol_state_host cnt_user_client_get_host_protocol_state(cnt_user_client* user);
bool cnt_user_client_is_active(cnt_user_client *user);
const char *cnt_user_client_state_to_string(cnt_user_client *user);
const char* cnt_user_client_client_protocol_to_string(cnt_user_client* user);
const char* cnt_user_client_host_protocol_to_string(cnt_user_client* user);
void cnt_user_client_close(cnt_user_client *user);
cnt_user_client *cnt_user_client_send(cnt_user_client *client, void *ptr, int byte_count);
int cnt_user_client_recv(cnt_user_client *client, void *ptr, int byte_count);

// Host
cnt_user_host *cnt_user_host_open(cnt_user_host *user, const char *host_ip, uint16_t port, uint32_t frequency);
cnt_user_host *cnt_user_host_shut_down(cnt_user_host *user);
cnt_net_engine_state_type cnt_user_host_get_state(cnt_user_host *user);
bool cnt_user_host_is_active(cnt_user_host *user);
const char *cnt_user_host_state_to_string(cnt_user_host *user);
void cnt_user_host_close(cnt_user_host *user);
cnt_user_host *cnt_user_host_broadcast(cnt_user_host *host, void *ptr, int byte_count);
cnt_user_host *cnt_user_host_send(cnt_user_host *host, cnt_sparse_index client_id, void *ptr, int byte_count);
cnt_user_host *cnt_user_host_kick(cnt_user_host *host, cnt_sparse_index client_id);
int cnt_user_host_recv(cnt_user_host *host, cnt_sparse_index *client_id, void *ptr, int byte_count);

// Tests
bool TEST_cnt_user_host_frame_spsc_queue();

#endif // !CNT_NET_INCLUDED
