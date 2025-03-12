#ifndef CNT_NET_INCLUDED
#define CNT_NET_INCLUDED

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_stdinc.h>

// We use uint32s for count and capacity, which gives us an unrealistic upper bound
// plus we do not need to deal with u64 vs u32 which makes the implementation easier
// For streams we use int. I accept it for since lz4 demands it.

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
};

enum cnt_protocol_state_client : uint8_t
{
	CNT_PROTOCOL_STATE_CLIENT_NONE = CNT_PROTOCOL_STATE_COMMON_NONE,
	CNT_PROTOCOL_STATE_CLIENT_REQUEST = 1,
	CNT_PROTOCOL_STATE_CLIENT_ANSWER = 3,
	CNT_PROTOCOL_STATE_CLIENT_OK = CNT_PROTOCOL_STATE_COMMON_OK,
};

enum cnt_protocol_state_host : uint8_t
{
	CNT_PROTOCOL_STATE_HOST_NONE = CNT_PROTOCOL_STATE_COMMON_NONE,
	CNT_PROTOCOL_STATE_HOST_CHALLENGE = 2,
	CNT_PROTOCOL_STATE_HOST_OK = CNT_PROTOCOL_STATE_COMMON_OK,
	CNT_PROTOCOL_STATE_HOST_RESOLUTION_REJECT = 5, // Everything bigger than 4 is out of the handshake - in other words, rejected
	CNT_PROTOCOL_STATE_HOST_KICKED = 6
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

// Host
cnt_host *cnt_host_open(cnt_host *host, cnt_sock *sock, uint32_t max_connections);
cnt_host *cnt_host_update(cnt_host *host);
cnt_ip *cnt_host_kick(cnt_host *host, cnt_ip *addr);
void cnt_host_close(cnt_host *host);

// Client
cnt_client *cnt_client_open(cnt_client *client, cnt_connection *connection);
cnt_client *cnt_client_send(cnt_client *client, cnt_stream *stream);
void cnt_client_close(cnt_client *client);

int example_client(void *);
int example_server(void *);

// Fix these
void cnt_start_up();
void cnt_tead_down();
#endif // !CNT_NET_INCLUDED
