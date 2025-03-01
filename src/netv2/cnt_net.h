#ifndef CNT_NET_INCLUDED
#define CNT_NET_INCLUDED

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_stdinc.h>

#define CNT_NULL_CHECK(var) SDL_assert((var) && #var " is null pointer")

struct cnt_client_on_host_to_dense_index
{
	uint32_t index;
};

struct cnt_client_on_host_index
{
	uint32_t index;
};

struct cnt_client_on_host_mapping
{
	cnt_client_on_host_to_dense_index *sparse;
	cnt_client_on_host_index *dense;

	uint32_t control_bit_mask;
	uint32_t free_head;

	uint32_t capacity;
	uint32_t count;
};

struct cnt_sock
{
	uint8_t handle[8];
};

struct cnt_addr
{
	uint8_t address[32];
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

struct cnt_addr_container
{
	cnt_addr *addresses;

	uint32_t capacity;
	uint32_t count;
};

enum cnt_protocol_state_common : uint8_t
{
	CNT_PROTOCOL_COMMON_STATE_NONE = 0,
	CNT_PROTOCOL_COMMON_STATE_OK = 6,
};

enum cnt_protocol_state_client : uint8_t
{
	CNT_PROTOCOL_STATE_CLIENT_NONE = CNT_PROTOCOL_COMMON_STATE_NONE,
	CNT_PROTOCOL_STATE_CLIENT_REQUEST = 1,
	CNT_PROTOCOL_STATE_CLIENT_ANSWER = 3,
	CNT_PROTOCOL_STATE_CLIENT_OK = CNT_PROTOCOL_COMMON_STATE_OK,
};

enum cnt_protocol_state_host : uint8_t
{
	CNT_PROTOCOL_STATE_HOST_NONE = CNT_PROTOCOL_COMMON_STATE_NONE,
	CNT_PROTOCOL_STATE_HOST_CHALLENGE = 2,
	CNT_PROTOCOL_STATE_HOST_RESOLUTION_REJECT = 4, // Maybe rename to kicked
	CNT_PROTOCOL_STATE_HOST_OK = CNT_PROTOCOL_COMMON_STATE_OK,
};

union cnt_protocol_client {
	uint32_t prefix;
	cnt_protocol_state_client state;

	// Client explicitly states its id for the server
	// The server reads id, compares secret, maybe compares ip
	// They work together to make resource resolution faster
	cnt_client_on_host_index id;

	uint8_t extra_payload_count;
	uint8_t extra_payload[16];
};

union cnt_protocol_host {
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
	cnt_addr dst;

	int error;
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
	cnt_addr_container destinations;
};

struct cnt_client
{
	cnt_connection connection;
	cnt_secret secret;
	cnt_protocol_state_client state;
};

struct cnt_client_state_on_host
{
	cnt_client_on_host_index id;
	cnt_secret secret;
	cnt_protocol_state_host protocol;
};

struct cnt_host
{
	cnt_client_on_host_mapping mapping;

	cnt_star star;

	cnt_client_state_on_host *client_states;
};

// Client on host mapping - yeehaw
void cnt_client_on_host_mapping_open(cnt_client_on_host_mapping* mapping, uint32_t capacity);
void cnt_client_on_host_mapping_close(cnt_client_on_host_mapping* mapping);

// Address
cnt_addr *cnt_addr_create(cnt_addr *address, const char *ip, uint16_t port);

// Socket
cnt_sock *cnt_sock_open(cnt_sock *sock, const char *ip, uint16_t port);
void cnt_sock_close(cnt_sock *sock);

// Stream
cnt_stream *cnt_stream_create(cnt_stream *stream, uint8_t *data, int count);
cnt_stream *cnt_stream_open(cnt_stream *stream, int capacity);
void cnt_stream_close(cnt_stream *stream);

// Transport - Maybe call it payload?
cnt_transport *cnt_transport_open(cnt_transport *transport, int capacity);
cnt_transport *cnt_transport_write(cnt_transport *transport, cnt_stream *stream);
cnt_transport *cnt_transport_read(cnt_transport *transport, cnt_stream *stream);
void cnt_transport_close(cnt_transport *transport);

// Compression
cnt_compression *cnt_compression_open(cnt_compression *compression, uint32_t capacity);
cnt_compression *cnt_compress(cnt_compression *compression, cnt_stream *stream);
cnt_stream *cnt_decompress(cnt_compression *compression, cnt_stream *stream);
void cnt_compression_close(cnt_compression *compression);

// Connection
cnt_connection *cnt_connection_open(cnt_connection *connection, cnt_sock *src, cnt_addr *dst);
cnt_connection *cnt_connection_send(cnt_connection *connection, cnt_stream *stream);
cnt_connection *cnt_connection_recv(cnt_connection *connection, cnt_stream *stream);
void cnt_connection_close(cnt_connection *connection);

// Star
cnt_star *cnt_star_open(cnt_star *star, cnt_sock *src, uint32_t max_connections);
cnt_star *cnt_star_send(cnt_star *star, cnt_stream *stream);
cnt_star *cnt_star_recv(cnt_star *star, cnt_addr *address, cnt_stream *stream);
cnt_star *cnt_star_add(cnt_star *star, cnt_addr *addr);
cnt_addr *cnt_star_remove(cnt_star *star, cnt_addr *addr);
void cnt_star_close(cnt_star *star);

// Host
cnt_host *cnt_host_open(cnt_host *host, cnt_sock *sock, uint32_t max_connections);
cnt_host *cnt_host_update(cnt_host *host);
cnt_addr *cnt_host_kick(cnt_host *host, cnt_addr *addr);
void cnt_host_close(cnt_host *host);

// Client
cnt_client *cnt_client_open(cnt_client *client, cnt_connection *connection);
cnt_protocol_client *cnt_client_update(cnt_client *client, cnt_protocol_client *procotol);
void cnt_client_close(cnt_client *client);

#endif // !CNT_NET_INCLUDED
