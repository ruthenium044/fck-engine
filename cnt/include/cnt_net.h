#ifndef CNT_NET_INCLUDED
#define CNT_NET_INCLUDED

#include <fckc_inttypes.h>

#define CNT_ANY_IP "0.0.0.0"
#define CNT_ANY_PORT 0

// Just exists for type-safety, do not extend
typedef struct cnt_sparse_index
{
	fckc_u32 index;
} cnt_sparse_index;

// Just exists for type-safety, do not extend
typedef struct cnt_dense_index
{
	fckc_u32 index;
} cnt_dense_index;

typedef struct cnt_sparse_list
{
	// This shit is still kind of confusing
	// sparse elements POINT to a dense array element
	// dense elements POINT to a sparse array element
	// TODO: Introduce something like a cnt_access_index for the user since sparse index is doing two jobs now
	cnt_sparse_index* sparse;
	cnt_dense_index* dense;

	fckc_u32 control_bit_mask;
	fckc_u32 free_head;

	fckc_u32 capacity;
	fckc_u32 count;
} cnt_sparse_list;

typedef struct cnt_sock
{
	fckc_u8 handle[8];
} cnt_sock;

typedef struct cnt_ip
{
	fckc_u8 address[32];
} cnt_ip;

typedef struct cnt_message_64_bytes
{
	cnt_ip ip;

	fckc_u8 payload_count;
	fckc_u8 payload[64];
} cnt_message_64_bytes;

// It is actually a stack, but fuck it, we want to send the newest messages AS FAST AS POSSIBLE
// We might drop messages anyway
typedef struct cnt_message_64_bytes_queue
{
	cnt_message_64_bytes* messages;

	fckc_u32 count;
	fckc_u32 capacity;
} cnt_message_64_bytes_queue;

typedef struct cnt_stream
{
	// We make these int since LZ4 prefers it... I guess
	int at;
	int capacity;

	fckc_u8* data;
} cnt_stream;

typedef struct cnt_transport
{
	cnt_stream stream;
} cnt_transport;

typedef struct cnt_compression
{
	cnt_stream stream;
} cnt_compression;

typedef struct cnt_ip_container
{
	cnt_ip* addresses;

	fckc_u32 capacity;
	fckc_u32 count;
} cnt_ip_container;

typedef enum cnt_protocol_state_common
{
	// At NO time the protocol shall be 0, this flag only exists for debugging
	CNT_PROTOCOL_STATE_COMMON_NONE = 0,
	CNT_PROTOCOL_STATE_COMMON_OK = 4,
	CNT_PROTOCOL_STATE_COMMON_DISCONNECT = 6,
	CNT_PROTOCOL_STATE_COMMON_KICKED = 7,
} cnt_protocol_state_common;

typedef enum cnt_protocol_state_client
{
	CNT_PROTOCOL_STATE_CLIENT_NONE = CNT_PROTOCOL_STATE_COMMON_NONE,
	CNT_PROTOCOL_STATE_CLIENT_REQUEST = 1,
	CNT_PROTOCOL_STATE_CLIENT_ANSWER = 3,
	CNT_PROTOCOL_STATE_CLIENT_OK = CNT_PROTOCOL_STATE_COMMON_OK,
	CNT_PROTOCOL_STATE_CLIENT_DISCONNECT = CNT_PROTOCOL_STATE_COMMON_DISCONNECT,
	CNT_PROTOCOL_STATE_CLIENT_KICKED = CNT_PROTOCOL_STATE_COMMON_KICKED,
} cnt_protocol_state_client;

typedef enum cnt_protocol_state_host
{
	CNT_PROTOCOL_STATE_HOST_NONE = CNT_PROTOCOL_STATE_COMMON_NONE,
	CNT_PROTOCOL_STATE_HOST_CHALLENGE = 2,
	CNT_PROTOCOL_STATE_HOST_OK = CNT_PROTOCOL_STATE_COMMON_OK,
	CNT_PROTOCOL_STATE_HOST_RESOLUTION_REJECT = 5, // Everything bigger than 4 is out of the handshake - in other words, rejected
	CNT_PROTOCOL_STATE_HOST_DISCONNECT = CNT_PROTOCOL_STATE_COMMON_DISCONNECT,
	CNT_PROTOCOL_STATE_HOST_KICKED = CNT_PROTOCOL_STATE_COMMON_KICKED,
} cnt_protocol_state_host;

typedef struct cnt_protocol_client
{
	fckc_u32 prefix;
	cnt_protocol_state_client state;

	// Client explicitly states its id for the server
	// The server reads id, compares secret, maybe compares ip
	// They work together to make resource resolution faster
	cnt_sparse_index id;

	fckc_u8 extra_payload_count;
	fckc_u8 extra_payload[16];
} cnt_protocol_client;

typedef struct cnt_protocol_host
{
	fckc_u32 prefix;
	cnt_protocol_state_host state;

	// The host propagates the client's id implicitly within the payload at some point
	// during the handshake. The client needs to remember it. It could and should become part of the challenge
	fckc_u8 extra_payload_count;
	fckc_u8 extra_payload[16];
} cnt_protocol_host;

typedef struct cnt_connection
{
	cnt_sock src;
	cnt_ip dst;
} cnt_connection;

typedef enum cnt_secret_state
{
	CNT_SECRET_STATE_EMPTY,
	CNT_SECRET_STATE_OUTDATED,
	CNT_SECRET_STATE_ACCEPTED,
	CNT_SECRET_STATE_REJECTED
} cnt_secret_state;

typedef struct cnt_secret
{
	fckc_u16 public_value;
	fckc_u16 private_value;
	cnt_secret_state state;
} cnt_secret;

typedef struct cnt_star
{
	cnt_sock sock;
	cnt_ip_container destinations;
} cnt_star;

typedef struct cnt_client
{
	cnt_sparse_index id_on_host;
	cnt_connection connection;
	cnt_secret secret;

	cnt_protocol_state_client protocol;
	cnt_protocol_state_host host_protocol;

	fckc_u8 attempts;
	// Disconnect
	// uint64_t timestamp;
} cnt_client;

typedef struct cnt_client_on_host
{
	cnt_sparse_index id;
	cnt_secret secret;

	cnt_protocol_state_host protocol;
	fckc_u8 attempts;
	// Disconnect
	// uint64_t timestamp;
} cnt_client_on_host;

typedef struct cnt_client_on_host_kicked
{
	fckc_u64 timestamp;
	cnt_sparse_index id;
} cnt_client_on_host_kicked;

typedef struct cnt_host
{
	fckc_u64 recv_count;
	fckc_u64 send_count;

	cnt_sparse_list mapping;
	cnt_ip* ip_lookup;
	cnt_client_on_host* client_states;
} cnt_host;

typedef struct cnt_user_client_frame
{
	fckc_u8 command_code;
	fckc_u32 count;
	fckc_u8 data[1];
} cnt_user_client_frame;

typedef struct cnt_queue_header
{
	fckc_u32 head;
	fckc_u32 tail;
	fckc_u32 capacity;
} cnt_queue_header;

typedef struct cnt_list_header
{
	fckc_u32 count;
	fckc_u32 capacity;
} cnt_list_header;

typedef struct cnt_user_client_frame_queue
{
	cnt_queue_header header;

	// Needs to be last
	cnt_user_client_frame* frames[1];
} cnt_user_client_frame_queue;

typedef struct cnt_user_client_frame_spsc_queue
{
	// Can be nullptr, &queues[0] or &queues[1]
	// Shared
	cnt_user_client_frame_queue* active;

	// Internal
	cnt_user_client_frame_queue* queues[2];
	fckc_u8 current_inactive;
} cnt_user_client_frame_spsc_queue;

typedef struct cnt_user_host_frame
{
	fckc_u32 count;
	cnt_sparse_index client_id;

	fckc_u8 data[1];
} cnt_user_host_frame;

typedef struct cnt_user_host_frame_queue
{
	cnt_queue_header header;

	// Needs to be last
	cnt_user_host_frame* frames[1];
} cnt_user_host_frame_queue;

typedef struct cnt_user_host_frame_spsc_queue
{
	// Can be nullptr, &queues[0] or &queues[1]
	// Shared
	cnt_user_host_frame_queue* active;

	// Internal
	cnt_user_host_frame_queue* queues[2];
	fckc_u8 current_inactive;
} cnt_user_host_frame_spsc_queue;

typedef enum cnt_user_client_command_type
{
	CNT_USER_CLIENT_COMMAND_TYPE_QUIT,
	CNT_USER_CLIENT_COMMAND_TYPE_RESTART
} cnt_user_client_command_type;

typedef struct cnt_user_client_command
{
	cnt_user_client_command_type type;
} cnt_user_client_command;

typedef struct cnt_user_client_command_queue
{
	cnt_queue_header header;

	cnt_user_client_command commands[1];
} cnt_user_client_command_queue;

typedef struct cnt_user_client_command_spsc_queue
{
	cnt_user_client_command_queue* active;
	cnt_user_client_command_queue* queues[2];
	fckc_u8 current_inactive;
} cnt_user_client_command_spsc_queue;

typedef enum cnt_user_host_command_type
{
	CNT_USER_HOST_COMMAND_TYPE_QUIT,
	CNT_USER_HOST_COMMAND_TYPE_RESTART,
	CNT_USER_HOST_COMMAND_TYPE_KICK
} cnt_user_host_command_type;

typedef struct cnt_user_host_command_common
{
	cnt_user_client_command_type type;
} cnt_user_host_command_common;

typedef struct cnt_user_host_command_kick
{
	cnt_user_client_command_type type;
	cnt_sparse_index client;
} cnt_user_host_command_kick;

typedef union cnt_user_host_command {
	cnt_user_host_command_type type;
	cnt_user_host_command_common common;
	cnt_user_host_command_kick kick;
} cnt_user_host_command;

typedef struct cnt_user_host_command_queue
{
	cnt_queue_header header;
	cnt_user_host_command commands[1];
} cnt_user_host_command_queue;

typedef struct cnt_user_host_command_spsc_queue
{
	cnt_user_host_command_queue* active;
	cnt_user_host_command_queue* queues[2];
	fckc_u8 current_inactive;
} cnt_user_host_command_spsc_queue;

typedef struct cnt_user_frequency
{
	SDL_AtomicU32 ms;
} cnt_user_frequency;

typedef struct cnt_net_engine_state
{
	SDL_AtomicU32 state;
} cnt_net_engine_state;

typedef struct cnt_client_state
{
	SDL_AtomicU32 state;
} cnt_client_state;

typedef struct cnt_host_state
{
	SDL_AtomicU32 state;
} cnt_host_state;

typedef struct cnt_client_id_on_host
{
	SDL_AtomicU32 id;
} cnt_client_id_on_host;

typedef enum cnt_net_engine_state_type
{
	CNT_NET_ENGINE_STATE_TYPE_NONE = 0, // IDK what to do in this case, tbh. I also do not really care
	CNT_NET_ENGINE_STATE_TYPE_USER_INITALISED = 1,
	CNT_NET_ENGINE_STATE_TYPE_OPENING = 2,
	CNT_NET_ENGINE_STATE_TYPE_RUNNING = 3,
	CNT_NET_ENGINE_STATE_TYPE_SHUTTING_DOWN = 4,
	CNT_NET_ENGINE_STATE_TYPE_CLOSED = 5
} cnt_net_engine_state_type;

typedef struct cnt_user_client
{
	cnt_user_client_frame_spsc_queue send_queue;
	cnt_user_client_frame_spsc_queue recv_queue;

	cnt_user_client_command_spsc_queue command_queue;

	cnt_user_frequency frequency;

	cnt_net_engine_state net_engine_state;

	cnt_host_state state_on_host;
	cnt_client_state protocol_state;
	cnt_client_id_on_host client_id_on_host;

	const char* host_ip;
	fckc_u16 host_port;
} cnt_user_client;

typedef struct cnt_user_host_client_list
{
	cnt_list_header header;

	cnt_client_on_host clients[1];
} cnt_user_host_client_list;

typedef struct cnt_user_host_client_spsc_list
{
	cnt_user_host_client_list* active;

	cnt_user_host_client_list* lists[2];

	SDL_AtomicU32 lock;

	// Read only
	fckc_u32 capacity;

	// Internal
	fckc_u8 current_inactive;
} cnt_user_host_client_spsc_list;

typedef struct cnt_user_host
{
	cnt_user_host_frame_spsc_queue send_queue;
	cnt_user_host_frame_spsc_queue recv_queue;

	cnt_user_host_command_spsc_queue command_queue;

	cnt_user_host_client_spsc_list client_list;

	cnt_user_frequency frequency;

	cnt_net_engine_state net_engine_state;

	const char* host_ip;
	fckc_u16 host_port;
	fckc_u32 max_clients;
} cnt_user_host;


// User Realm
// Utility
const char* cnt_net_engine_state_type_to_string(cnt_net_engine_state_type state);
const char* cnt_protocol_state_host_to_string(cnt_protocol_state_host state);
const char* cnt_protocol_state_client_to_string(cnt_protocol_state_client state);

// Client
cnt_user_client* cnt_user_client_open(cnt_user_client* user, const char* host_ip, fckc_u16 port, fckc_u16 frequency);
cnt_user_client* cnt_user_client_shut_down(cnt_user_client* user);
void cnt_user_client_close(cnt_user_client* user);

cnt_user_client* cnt_user_client_send(cnt_user_client* client, void* ptr, int byte_count);
int cnt_user_client_recv(cnt_user_client* client, void* ptr, int byte_count);
cnt_user_client* cnt_user_client_keep_alive(cnt_user_client* client);

int cnt_user_client_is_active(cnt_user_client* user);
cnt_net_engine_state_type cnt_user_client_get_state(cnt_user_client* user);
cnt_protocol_state_client cnt_user_client_get_client_protocol_state(cnt_user_client* user);
cnt_protocol_state_host cnt_user_client_get_host_protocol_state(cnt_user_client* user);
cnt_sparse_index cnt_user_client_get_client_id_on_host(cnt_user_client* user);
const char* cnt_user_client_state_to_string(cnt_user_client* user);
const char* cnt_user_client_client_protocol_to_string(cnt_user_client* user);
const char* cnt_user_client_host_protocol_to_string(cnt_user_client* user);


// Host
cnt_user_host* cnt_user_host_open(cnt_user_host* user, const char* host_ip, fckc_u16 port, fckc_u16 max_clients, fckc_u16 frequency);
cnt_user_host* cnt_user_host_shut_down(cnt_user_host* user);
void cnt_user_host_close(cnt_user_host* user);

cnt_user_host* cnt_user_host_broadcast(cnt_user_host* host, void* ptr, int byte_count);
cnt_user_host* cnt_user_host_send(cnt_user_host* host, cnt_sparse_index client_id, void* ptr, int byte_count);
int cnt_user_host_recv(cnt_user_host* host, cnt_sparse_index* client_id, void* ptr, int byte_count);
cnt_user_host* cnt_user_host_keep_alive(cnt_user_host* host);
cnt_user_host* cnt_user_host_kick(cnt_user_host* host, cnt_sparse_index client_id);

void cnt_user_host_client_list_lock(cnt_user_host* host);
cnt_client_on_host* cnt_user_host_client_list_get(cnt_user_host* host, fckc_u32* count);
void cnt_user_host_client_list_unlock(cnt_user_host* host);

int cnt_user_host_is_active(cnt_user_host* user);
cnt_net_engine_state_type cnt_user_host_get_state(cnt_user_host* user);
const char* cnt_user_host_state_to_string(cnt_user_host* user);


// Tests
int TEST_cnt_user_host_frame_spsc_queue();

#endif // !CNT_NET_INCLUDED