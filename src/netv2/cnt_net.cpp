#include "netv2/cnt_net.h"

#include <SDL3/SDL_atomic.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_thread.h>
#include <SDL3/SDL_timer.h>

#include <errno.h>
#include <string.h>

#include "lz4.h"
#include "shared/fck_checks.h"

// Internal declaration for the default client/host processes
int cnt_client_default_process(cnt_user_client *);
int cnt_host_default_process(cnt_user_host *);

// Windows
#ifdef _WIN32
#include <ws2tcpip.h>
struct cnt_sock_internal
{
	SOCKET handle;
};
struct cnt_ip_internal
{
	union {
		ADDRESS_FAMILY family; // Address family.
		sockaddr addr;         // base
		sockaddr_in in;        // ipv4
		sockaddr_in6 in6;      // ipv6
	};
	uint8_t addrlen; // Fucking asshole Win32 doesn't have length
};
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
struct cnt_sock_internal
{
	int handle;
};
struct cnt_ip_internal // Not tested on Linux, only MacOS
{
	union {
		struct
		{
			uint8_t addrlen;    // Addrlen is part of BSD header info
			sa_family_t family; // Address family.
		};
		sockaddr addr;    // base
		sockaddr_in in;   // ipv4
		sockaddr_in6 in6; // ipv6
	};
};
#endif

static_assert(sizeof(cnt_ip) >= sizeof(cnt_ip_internal), "User-facing IP address is not large enough to hold internal IP address");
static_assert(sizeof(cnt_sock) >= sizeof(cnt_sock_internal), "User-facing socket is not large enough to hold internal socket");

#if _WIN32
#include <winsock2.h>
static SDL_AtomicInt wsa_reference_counter = {0};
#endif // _WIN32

// TODO: Use it consistently
#define CNT_NULL_CHECK(var) SDL_assert((var) && #var " is null pointer")

#define CNT_FOR(type, var_name, count) for (type var_name = 0; (var_name) < (count); (var_name)++)
#define CNT_SIZEOF_SMALLER(a, b) sizeof(a) < sizeof(b) ? sizeof(a) : sizeof(b)

const char *cnt_protocol_info_name[] = {"CNT_PROTOCOL_STATE_COMMON_NONE",    "CNT_PROTOCOL_STATE_CLIENT_REQUEST",
                                        "CNT_PROTOCOL_STATE_HOST_CHALLENGE", "CNT_PROTOCOL_STATE_CLIENT_ANSWER",
                                        "CNT_PROTOCOL_STATE_COMMON_OK",      "CNT_PROTOCOL_STATE_HOST_RESOLUTION_REJECT",
                                        "CNT_PROTOCOL_STATE_HOST_KICKED"};

// This only works under the assumption that the user will never receive
// a tombstone sparse index to begin with. Let's pray
#define CNT_SPARSE_INDEX_INVALID UINT32_MAX
static_assert(CNT_SPARSE_INDEX_INVALID == UINT32_MAX);

void cnt_sparse_list_open(cnt_sparse_list *mapping, uint32_t capacity)
{
	const uint32_t maximum_possible_capacity = (CNT_SPARSE_INDEX_INVALID >> 1);

	CNT_NULL_CHECK(mapping);
	SDL_assert(capacity <= maximum_possible_capacity && "Capacity is too large. Stay below 0x7FFFFFFF");

	SDL_zerop(mapping);

	mapping->capacity = capacity;
	mapping->free_head = CNT_SPARSE_INDEX_INVALID;
	mapping->control_bit_mask = (CNT_SPARSE_INDEX_INVALID >> 1) + 1;

	mapping->sparse = (cnt_sparse_index *)SDL_malloc(sizeof(*mapping->sparse) * mapping->capacity);
	mapping->dense = (cnt_dense_index *)SDL_malloc(sizeof(*mapping->dense) * mapping->capacity);
	CNT_FOR(uint32_t, index, mapping->capacity)
	{
		mapping->sparse[index].index = CNT_SPARSE_INDEX_INVALID;
	}
}

void cnt_sparse_list_close(cnt_sparse_list *mapping)
{
	CNT_NULL_CHECK(mapping);

	SDL_free(mapping->sparse);
	SDL_free(mapping->dense);

	SDL_zerop(mapping);
}

void cnt_sparse_list_make_invalid(cnt_sparse_list *mapping, cnt_sparse_index *index)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(index);

	const uint32_t control_bit_mask = mapping->control_bit_mask;
	index->index = index->index | control_bit_mask;
}

void cnt_sparse_list_make_valid(cnt_sparse_list *mapping, cnt_sparse_index *index)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(index);

	const uint32_t control_bit_mask = mapping->control_bit_mask;
	index->index = (index->index & ~control_bit_mask);
}

bool cnt_sparse_list_exists(cnt_sparse_list *mapping, cnt_sparse_index *index)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(index);

	if (index->index >= mapping->capacity)
	{
		return false;
	}

	cnt_sparse_index *slot = &mapping->sparse[index->index];
	const uint32_t control_bit_mask = mapping->control_bit_mask;
	if ((slot->index & control_bit_mask) == control_bit_mask)
	{
		return false;
	}

	return true;
}

bool cnt_sparse_list_try_get_sparse(cnt_sparse_list *mapping, cnt_dense_index *index, cnt_sparse_index *out)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(out);

	if (index->index >= mapping->count)
	{
		return false;
	}

	cnt_dense_index *dense = &mapping->dense[index->index];
	out->index = dense->index;

	return true;
}

bool cnt_sparse_list_try_get_dense(cnt_sparse_list *mapping, cnt_sparse_index *index, cnt_dense_index *out)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(out);

	if (!cnt_sparse_list_exists(mapping, index))
	{
		out = nullptr;
		return false;
	}

	cnt_sparse_index *sparse = &mapping->sparse[index->index];
	out->index = sparse->index;

	return true;
}

bool cnt_sparse_list_create(cnt_sparse_list *mapping, cnt_sparse_index *out)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(out);

	SDL_zerop(out);

	if (mapping->count >= mapping->capacity)
	{
		return false;
	}

	uint32_t next_dense = mapping->count;

	bool has_free_slot = mapping->free_head != UINT32_MAX;
	if (has_free_slot)
	{
		cnt_sparse_index free_sparse_index;
		free_sparse_index.index = mapping->free_head;
		cnt_sparse_list_make_valid(mapping, &free_sparse_index);

		// Value stored in sparse[candidate] is invalid
		// We get the value, make it valid and assign it to free_head
		// By convenience, the free list head is always valid to use for indexing
		cnt_sparse_index *sparse_index = &mapping->sparse[free_sparse_index.index];

		mapping->free_head = sparse_index->index;

		sparse_index->index = next_dense;

		cnt_dense_index *dense_index = &mapping->dense[next_dense];
		dense_index->index = free_sparse_index.index;
	}
	else
	{
		// Create a new one. We can safely use the dense list count
		// If no free list item is available, it implies the sparse lista and dense list
		// have the same length! In other words, everything is used nicely
		cnt_sparse_index *sparse_index = &mapping->sparse[next_dense];
		sparse_index->index = next_dense;

		cnt_dense_index *dense_index = &mapping->dense[next_dense];
		dense_index->index = next_dense;
	}
	cnt_dense_index *dense = &mapping->dense[next_dense];
	out->index = dense->index;

	mapping->count = mapping->count + 1;
	return true;
}

bool cnt_sparse_list_remove(cnt_sparse_list *mapping, cnt_sparse_index *index)
{
	CNT_NULL_CHECK(mapping);

	if (!cnt_sparse_list_exists(mapping, index))
	{
		return false;
	}

	// Update mapping
	{
		uint32_t last_dense = mapping->count - 1;
		cnt_dense_index *last_dense_index = &mapping->dense[last_dense];

		// Last sparse is now pointing to the removed index
		cnt_sparse_index *last_sparse_index = &mapping->sparse[last_dense_index->index];
		last_sparse_index->index = index->index;

		// Last dense index is getting copied over to dense data of the removed index
		cnt_dense_index *dense_index = &mapping->dense[last_sparse_index->index];
		dense_index->index = last_dense_index->index;

		// Pop last element - We did a remove swap while maintaining the sparse-dense mapping
		mapping->count = last_dense;
	}

	// Update free list - Add the removed index (slot) to the free list by invalidating it
	// The index itself becomes the new head of the free list
	{
		cnt_sparse_index *sparse_index = &mapping->sparse[index->index];
		cnt_sparse_list_make_invalid(mapping, index);
		sparse_index->index = mapping->free_head;
		mapping->free_head = index->index;
	}
	return true;
}

// This is shit. Get rid of it.
void cnt_start_up()
{
#if _WIN32
	// TODO: Fix this non-sense up.
	int reference_counter = SDL_GetAtomicInt(&wsa_reference_counter);
	if (reference_counter == 0)
	{
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
	}
	SDL_AtomicIncRef(&wsa_reference_counter);
#endif // _WIN32
}

void cnt_tead_down()
{
#if _WIN32
	SDL_AtomicDecRef(&wsa_reference_counter);
	int reference_counter = SDL_GetAtomicInt(&wsa_reference_counter);
	SDL_assert(reference_counter >= 0);

	if (reference_counter == 0)
	{
		WSACleanup();
	}
#endif // _WIN32
}

typedef socklen_t cnt_socklen;
typedef struct sockaddr_storage cnt_sockaddr_storage;

static cnt_sock *copy_to_user_sock(cnt_sock_internal *sock, cnt_sock *dst)
{
	SDL_zerop(dst);
	SDL_memcpy(dst, sock, CNT_SIZEOF_SMALLER(*dst, *sock));

	return dst;
}

static cnt_sock_internal *copy_to_internal_sock(cnt_sock *sock, cnt_sock_internal *dst)
{
	SDL_zerop(dst);
	SDL_memcpy(dst, sock, CNT_SIZEOF_SMALLER(*dst, *sock));

	return dst;
}

static cnt_ip *copy_to_user_address(cnt_ip_internal *addr, cnt_ip *dst)
{
	SDL_zerop(dst);
	SDL_memcpy(dst, addr, CNT_SIZEOF_SMALLER(*dst, *addr));

	return dst;
}

static cnt_ip_internal *copy_to_internal_address(cnt_ip *addr, cnt_ip_internal *dst)
{
	SDL_zerop(dst);
	SDL_memcpy(dst, addr, CNT_SIZEOF_SMALLER(*dst, *addr));

	return dst;
}

static int close_socket(cnt_sock_internal *sock)
{
#ifdef _WIN32
	return closesocket(sock->handle);
#else
	return close(sock->handle);
#endif
}

static int last_socket_error(void)
{
#ifdef _WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}

static bool socket_is_valid(cnt_sock_internal *socket)
{
#ifdef _WIN32
	return socket->handle != INVALID_SOCKET;
#else
	return socket->handle != -1;
#endif
}

static char *create_socket_error(int rc)
{
#ifdef _WIN32
	WCHAR msgbuf[256];
	const DWORD bw = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, rc,
	                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
	                                msgbuf, SDL_arraysize(msgbuf), NULL);
	if (bw == 0)
	{
		return SDL_strdup("Unknown error");
	}
	return SDL_iconv_string("UTF-8", "UTF-16LE", (const char *)msgbuf, (((size_t)bw) + 1) * sizeof(WCHAR));
#else
	return SDL_strdup(strerror(rc));
#endif
}

static char *create_get_address_error_info_string(int rc)
{
#ifdef _WIN32
	return create_socket_error(rc); // same error codes.
#else
	return SDL_strdup((rc == EAI_SYSTEM) ? strerror(errno) : gai_strerror(rc));
#endif
}

static int make_socket_non_blocking(cnt_sock_internal *sock)
{
#ifdef _WIN32
	DWORD one = 1;
	return ioctlsocket(sock->handle, FIONBIO, &one);
#else
	return fcntl(sock->handle, F_SETFL, fcntl(sock->handle, F_GETFL, 0) | O_NONBLOCK);
#endif
}

static bool would_block(const int err)
{
#ifdef _WIN32
	return (err == WSAEWOULDBLOCK) ? true : false;
#else
	return ((err == EWOULDBLOCK) || (err == EAGAIN) || (err == EINPROGRESS)) ? true : false;
#endif
}

static bool cnt_ip_is_empty(cnt_ip *ip)
{
	CNT_NULL_CHECK(ip);

	cnt_ip_internal ip_internal;
	copy_to_internal_address(ip, &ip_internal);

	return ip_internal.addrlen == 0;
}

static bool cnt_ip_equals(cnt_ip *a, cnt_ip *b)
{
	if (a == nullptr || b == nullptr)
	{
		return false;
	}

	cnt_ip_internal lhs;
	copy_to_internal_address(a, &lhs);
	cnt_ip_internal rhs;
	copy_to_internal_address(b, &rhs);

	if (lhs.addrlen != rhs.addrlen)
	{
		return false;
	}

	return SDL_memcmp(&lhs.addr, &rhs.addr, lhs.addrlen) == 0;
}

cnt_ip *cnt_ip_create(cnt_ip *address, const char *ip, uint16_t port)
{
	SDL_assert(address && "Address is null pointer");

	// We only accept
	uint8_t buffer[128];
	int result = inet_pton(AF_INET, ip, &buffer);
	CHECK_ERROR(result == 1, "Cannot convert address from text to binary - invalid arguments", return {});

	cnt_ip_internal result_address;
	SDL_zero(result_address);

	result_address.in.sin_family = AF_INET;
	result_address.in.sin_addr = *(in_addr *)buffer; // Prefer memcpy
	result_address.in.sin_port = port;
	result_address.addrlen = sizeof(sockaddr_in);

	copy_to_user_address(&result_address, address);

	return address;
}

static cnt_ip_container *cnt_ip_container_open(cnt_ip_container *container, uint32_t capacity)
{
	SDL_assert(container && "Container is null pointer");

	container->addresses = (cnt_ip *)SDL_malloc(sizeof(*container->addresses) * capacity);
	container->capacity = capacity;
	container->count = 0;
	return container;
}

static cnt_ip *cnt_ip_container_add(cnt_ip_container *container, cnt_ip *address)
{
	SDL_assert(container && "Container is null pointer");

	// Add new address if not found!
	uint32_t at = container->count;
	SDL_memcpy(container->addresses + at, address, sizeof(*address));
	container->count = at + 1;
	return container->addresses + at;
}

bool cnt_ip_container_index_of(cnt_ip_container *container, cnt_ip *address, uint32_t *index)
{
	SDL_assert(container && "Container is null pointer");

	CNT_FOR(uint32_t, i, container->count)
	{
		if (cnt_ip_equals(container->addresses + i, address))
		{
			*index = i;
			return true;
		}
	}

	return false;
}

static void cnt_ip_container_remove_swap_at(cnt_ip_container *container, uint32_t index)
{
	SDL_assert(container && "Container is null pointer");

	uint32_t last = container->count - 1;

	SDL_memcpy(container->addresses + index, container->addresses + last, sizeof(*container->addresses));
	container->count = last;
}

static void cnt_ip_container_clear(cnt_ip_container *container)
{
	SDL_assert(container && "Container is null pointer");

	container->count = 0;
}

static void cnt_ip_container_close(cnt_ip_container *container)
{
	SDL_assert(container && "Container is null pointer");

	SDL_free(container->addresses);
	SDL_zerop(container);
}

cnt_sock *cnt_sock_open(cnt_sock *sock, const char *ip, uint16_t port)
{
	SDL_assert(sock && "Sock is null pointer");

	cnt_ip address;
	if (cnt_ip_create(&address, ip, port) == nullptr)
	{
		return nullptr;
	}

	cnt_ip_internal addr_internal;
	copy_to_internal_address(&address, &addr_internal);

	cnt_sock_internal sock_internal;
	sock_internal.handle = socket(addr_internal.family, SOCK_DGRAM, 0);
	CHECK_CRITICAL(socket_is_valid(&sock_internal), "Failed to create socket", return nullptr);

	const bool non_block_sucess = make_socket_non_blocking(&sock_internal) >= 0;
	CHECK_CRITICAL(non_block_sucess, "Failed to make socket non-blocking", close_socket(&sock_internal); return nullptr);

	const int binding_failed = bind(sock_internal.handle, &addr_internal.addr, addr_internal.addrlen);
	CHECK_CRITICAL(binding_failed != -1, "Failed to bint socket", close_socket(&sock_internal); return nullptr);

	copy_to_user_sock(&sock_internal, sock);

	return sock;
}

void cnt_sock_close(cnt_sock *sock)
{
	SDL_assert(sock && "Sock is null pointer");

	cnt_sock_internal sock_internal;
	copy_to_internal_sock(sock, &sock_internal);
	close_socket(&sock_internal);

	SDL_zerop(sock);
}

cnt_message_64_bytes_queue *cnt_message_queue_64_bytes_open(cnt_message_64_bytes_queue *queue, uint32_t capacity)
{
	CNT_NULL_CHECK(queue);

	SDL_zerop(queue);

	queue->capacity = capacity;
	queue->messages = (cnt_message_64_bytes *)SDL_malloc(sizeof(*queue->messages) * capacity);

	return queue;
}

cnt_message_64_bytes *cnt_message_queue_64_bytes_push(cnt_message_64_bytes_queue *queue, cnt_message_64_bytes const *message)
{
	CNT_NULL_CHECK(queue);
	if (queue->count >= queue->capacity)
	{
		return nullptr;
	}

	SDL_memcpy(queue->messages + queue->count, message, sizeof(*queue->messages));

	queue->count = queue->count + 1;

	return queue->messages + queue->count;
}

void cnt_message_queue_64_bytes_clear(cnt_message_64_bytes_queue *queue)
{
	CNT_NULL_CHECK(queue);

	queue->count = 0;
}

void cnt_message_queue_64_bytes_close(cnt_message_64_bytes_queue *queue)
{
	CNT_NULL_CHECK(queue);

	SDL_free(queue->messages);

	SDL_zerop(queue);
}

cnt_stream *cnt_stream_create(cnt_stream *stream, uint8_t *data, int capacity)
{
	CNT_NULL_CHECK(stream);

	stream->data = data;
	stream->capacity = capacity;
	stream->at = 0;
	return stream;
}

// Find better name - Buffer with at set to capacity
cnt_stream *cnt_stream_create_full(cnt_stream *stream, uint8_t *data, int capacity)
{
	CNT_NULL_CHECK(stream);

	stream->data = data;
	stream->capacity = capacity;
	stream->at = capacity;
	return stream;
}

cnt_stream *cnt_stream_open(cnt_stream *stream, int capacity)
{
	CNT_NULL_CHECK(stream);

	stream->data = (uint8_t *)SDL_malloc(capacity);
	stream->capacity = capacity;
	stream->at = 0;
	return stream;
}

cnt_stream *cnt_stream_clear(cnt_stream *stream)
{
	CNT_NULL_CHECK(stream);

	stream->at = 0;

	return stream;
}

void cnt_stream_close(cnt_stream *stream)
{
	SDL_assert(stream && "Stream is null pointer");

	SDL_free(stream->data);
	SDL_zerop(stream);
}

static void cnt_stream_append(cnt_stream *serialiser, uint8_t *value, uint16_t count)
{
	uint8_t *at = serialiser->data + serialiser->at;

	SDL_memcpy(at, value, count);

	serialiser->at = serialiser->at + (sizeof(*value) * count);
}

static void cnt_stream_string_length(cnt_stream *serialiser, size_t *count)
{
	uint8_t const *at = serialiser->data + serialiser->at;

	size_t max_len = serialiser->capacity - serialiser->at;
	*count = SDL_strnlen((const char *)at, max_len);
}

static void cnt_stream_peek_string(cnt_stream *serialiser, uint8_t *value, uint16_t count)
{
	uint8_t *at = serialiser->data + serialiser->at;

	SDL_memcpy(value, at, count);
}
static void cnt_stream_peek_uint8(cnt_stream *serialiser, uint8_t *value)
{
	uint8_t *at = serialiser->data + serialiser->at;

	*value = *at;
}
static void cnt_stream_peek_uint16(cnt_stream *serialiser, uint16_t *value)
{
	uint8_t *at = serialiser->data + serialiser->at;

	*value = ((uint16_t)at[0] << 0)    // 0xFF000000
	         | ((uint16_t)at[1] << 8); // 0x00FF0000
}
static void cnt_stream_peek_uint32(cnt_stream *serialiser, uint32_t *value)
{
	uint8_t *at = serialiser->data + serialiser->at;

	*value = (uint32_t(at[0]) << 0)     // 0xFF000000
	         | (uint32_t(at[1]) << 8)   // 0x00FF0000
	         | (uint32_t(at[2]) << 16)  // 0x0000FF00
	         | (uint32_t(at[3]) << 24); // 0x000000FF
}
static void cnt_stream_peek_uint64(cnt_stream *serialiser, uint64_t *value)
{
	uint8_t *at = serialiser->data + serialiser->at;

	*value = ((uint64_t)at[0] << 0ull)     // 0xFF00000000000000
	         | ((uint64_t)at[1] << 8ull)   // 0x00FF000000000000
	         | ((uint64_t)at[2] << 16ull)  // 0x0000FF0000000000
	         | ((uint64_t)at[3] << 24ull)  // 0x000000FF00000000
	         | ((uint64_t)at[4] << 32ull)  // 0x00000000FF000000
	         | ((uint64_t)at[5] << 40ull)  // 0x0000000000FF0000
	         | ((uint64_t)at[6] << 48ull)  // 0x000000000000FF00
	         | ((uint64_t)at[7] << 56ull); // 0x00000000000000FF
}

static void cnt_stream_read_string(cnt_stream *serialiser, void *value, uint16_t count)
{
	uint8_t *at = serialiser->data + serialiser->at;

	SDL_memcpy(value, at, count);

	serialiser->at = serialiser->at + count;
}
static void cnt_stream_read_uint8(cnt_stream *serialiser, uint8_t *value)
{

	uint8_t *at = serialiser->data + serialiser->at;

	*value = *at;
	serialiser->at = serialiser->at + sizeof(*value);
}
static void cnt_stream_read_uint16(cnt_stream *serialiser, uint16_t *value)
{
	uint8_t *at = serialiser->data + serialiser->at;

	*value = ((uint16_t)at[0] << 0)    // 0xFF000000
	         | ((uint16_t)at[1] << 8); // 0x00FF0000

	serialiser->at = serialiser->at + sizeof(*value);
}
static void cnt_stream_read_uint32(cnt_stream *serialiser, uint32_t *value)
{
	uint8_t *at = serialiser->data + serialiser->at;

	*value = (uint32_t(at[0]) << 0)     // 0xFF000000
	         | (uint32_t(at[1]) << 8)   // 0x00FF0000
	         | (uint32_t(at[2]) << 16)  // 0x0000FF00
	         | (uint32_t(at[3]) << 24); // 0x000000FF

	serialiser->at = serialiser->at + sizeof(*value);
}
static void cnt_stream_read_uint64(cnt_stream *serialiser, uint64_t *value)
{
	uint8_t *at = serialiser->data + serialiser->at;

	*value = ((uint64_t)at[0] << 0ull)     // 0xFF00000000000000
	         | ((uint64_t)at[1] << 8ull)   // 0x00FF000000000000
	         | ((uint64_t)at[2] << 16ull)  // 0x0000FF0000000000
	         | ((uint64_t)at[3] << 24ull)  // 0x000000FF00000000
	         | ((uint64_t)at[4] << 32ull)  // 0x00000000FF000000
	         | ((uint64_t)at[5] << 40ull)  // 0x0000000000FF0000
	         | ((uint64_t)at[6] << 48ull)  // 0x000000000000FF00
	         | ((uint64_t)at[7] << 56ull); // 0x00000000000000FF

	serialiser->at = serialiser->at + sizeof(*value);
}

static void cnt_stream_write_string(cnt_stream *serialiser, void const *value, uint16_t count)
{
	uint8_t *at = serialiser->data + serialiser->at;

	SDL_memcpy(at, value, count);

	serialiser->at = serialiser->at + count;
}
static void cnt_stream_write_uint8(cnt_stream *serialiser, uint8_t value)
{
	uint8_t *at = serialiser->data + serialiser->at;

	*at = value;

	serialiser->at = serialiser->at + sizeof(value);
}
static void cnt_stream_write_uint16(cnt_stream *serialiser, uint16_t value)
{
	uint8_t *at = serialiser->data + serialiser->at;

	at[0] = (uint8_t)((value >> 0) & 0xFF);
	at[1] = (uint8_t)((value >> 8) & 0xFF);

	serialiser->at = serialiser->at + sizeof(value);
}
static void cnt_stream_write_uint32(cnt_stream *serialiser, uint32_t value)
{
	uint8_t *at = serialiser->data + serialiser->at;

	at[0] = (uint8_t)((value >> 0) & 0xFF);
	at[1] = (uint8_t)((value >> 8) & 0xFF);
	at[2] = (uint8_t)((value >> 16) & 0xFF);
	at[3] = (uint8_t)((value >> 24) & 0xFF);

	serialiser->at = serialiser->at + sizeof(value);
}
static void cnt_stream_write_uint64(cnt_stream *serialiser, uint64_t value)
{
	uint8_t *at = serialiser->data + serialiser->at;

	at[0] = (uint8_t)((value >> 0) & 0xFF);
	at[1] = (uint8_t)((value >> 8) & 0xFF);
	at[2] = (uint8_t)((value >> 16) & 0xFF);
	at[3] = (uint8_t)((value >> 24) & 0xFF);
	at[4] = (uint8_t)((value >> 32) & 0xFF);
	at[5] = (uint8_t)((value >> 40) & 0xFF);
	at[6] = (uint8_t)((value >> 48) & 0xFF);
	at[7] = (uint8_t)((value >> 56) & 0xFF);

	serialiser->at = serialiser->at + sizeof(value);
}

cnt_transport *cnt_transport_open(cnt_transport *transport, int capacity)
{
	SDL_assert(transport && "Transport is null pointer");

	cnt_stream_open(&transport->stream, capacity);
	return transport;
}

void cnt_transport_close(cnt_transport *transport)
{
	SDL_assert(transport && "Transport is null pointer");

	cnt_stream_close(&transport->stream);
	SDL_zerop(transport);
}

cnt_compression *cnt_compression_open(cnt_compression *compression, uint32_t capacity)
{
	SDL_assert(compression && "Compression is null pointer");

	cnt_stream_open(&compression->stream, capacity);
	return compression;
}

cnt_compression *cnt_compress(cnt_compression *compression, cnt_stream *stream)
{
	SDL_assert(compression && "Compression is null pointer");
	SDL_assert(stream && "Stream is null pointer");

	int result = LZ4_compress_default((char *)stream->data, (char *)compression->stream.data, stream->at, compression->stream.capacity);
	if (result >= 0)
	{
		compression->stream.at = result;
	}
	return compression;
}

cnt_stream *cnt_decompress(cnt_compression *compression, cnt_stream *stream)
{
	SDL_assert(compression && "Compression is null pointer");
	SDL_assert(stream && "Stream is null pointer");

	int result = LZ4_decompress_safe((char *)compression->stream.data, (char *)stream->data, compression->stream.at, stream->capacity);
	if (result >= 0)
	{
		stream->at = result;
	}
	return stream;
}

void cnt_compression_close(cnt_compression *compression)
{
	SDL_assert(compression && "Compression is null pointer");

	cnt_stream_close(&compression->stream);
	SDL_zerop(compression);
}

static void cnt_protocol_client_write(cnt_protocol_client *packet, cnt_stream *stream)
{
	cnt_stream_write_uint32(stream, packet->prefix);
	cnt_stream_write_uint8(stream, (uint8_t)packet->state);
	cnt_stream_write_uint32(stream, packet->id.index);

	cnt_stream_write_uint8(stream, packet->extra_payload_count);
	cnt_stream_write_string(stream, packet->extra_payload, packet->extra_payload_count);
}

static void cnt_protocol_client_read(cnt_protocol_client *packet, cnt_stream *stream)
{
	cnt_stream_read_uint32(stream, &packet->prefix);
	cnt_stream_read_uint8(stream, (uint8_t *)&packet->state);
	cnt_stream_read_uint32(stream, &packet->id.index);

	cnt_stream_read_uint8(stream, &packet->extra_payload_count);
	cnt_stream_read_string(stream, packet->extra_payload, packet->extra_payload_count);
}

static void cnt_protocol_host_write(cnt_protocol_host *packet, cnt_stream *stream)
{
	cnt_stream_write_uint32(stream, packet->prefix);
	cnt_stream_write_uint8(stream, (uint8_t)packet->state);
	cnt_stream_write_uint8(stream, packet->extra_payload_count);
	cnt_stream_write_string(stream, packet->extra_payload, packet->extra_payload_count);
}

static void cnt_protocol_host_read(cnt_protocol_host *packet, cnt_stream *stream)
{
	cnt_stream_read_uint32(stream, &packet->prefix);
	cnt_stream_read_uint8(stream, (uint8_t *)&packet->state);
	cnt_stream_read_uint8(stream, &packet->extra_payload_count);
	cnt_stream_read_string(stream, packet->extra_payload, packet->extra_payload_count);
}

static void cnt_protocol_secret_write(cnt_secret *secret, cnt_stream *stream)
{
	cnt_stream_write_uint8(stream, (uint8_t)secret->state);

	cnt_stream_write_uint16(stream, secret->public_value);
}

static void cnt_protocol_secret_read(cnt_secret *secret, cnt_stream *stream)
{
	cnt_stream_read_uint8(stream, (uint8_t *)&secret->state);

	cnt_stream_read_uint16(stream, &secret->public_value);
}

cnt_connection *cnt_connection_from_socket(cnt_connection *connection, cnt_sock *src)
{
	SDL_assert(connection && "Connection is null");
	SDL_assert(src && "Socket (src) is null pointer");

	SDL_memcpy(&connection->src, src, sizeof(*src));
	SDL_zero(connection->dst);

	return connection;
}

cnt_connection *cnt_connection_set_destination(cnt_connection *connection, cnt_ip *dst)
{
	SDL_assert(connection && "Connection is null");
	SDL_assert(dst && "Socket (src) is null pointer");

	SDL_memcpy(&connection->dst, dst, sizeof(*dst));

	return connection;
}

cnt_connection *cnt_connection_open(cnt_connection *connection, cnt_sock *src, cnt_ip *dst)
{
	SDL_assert(connection && "Connection is null");
	SDL_assert(src && "Socket (src) is null pointer");
	SDL_assert(dst && "Address (dst) is null pointer");

	SDL_memcpy(&connection->src, src, sizeof(*src));
	SDL_memcpy(&connection->dst, dst, sizeof(*dst));
	return connection;
}

cnt_connection *cnt_connection_send(cnt_connection *connection, cnt_stream *stream)
{
	SDL_assert(connection && "Connection is null pointer");
	SDL_assert(stream && "Stream is null pointer");

	cnt_sock_internal sock;
	copy_to_internal_sock(&connection->src, &sock);
	cnt_ip_internal addr;
	copy_to_internal_address(&connection->dst, &addr);

	int result = sendto(sock.handle, (char *)stream->data, stream->at, 0, &addr.addr, addr.addrlen);

	if (result == -1)
	{
		const int err = last_socket_error();
		if (would_block(err))
		{
			return connection;
		}
	}
	stream->at = result;
	return connection;
}

static void query_address(cnt_sockaddr_storage *socket_address_storage, cnt_ip *out_address)
{
	cnt_ip_internal address = *(cnt_ip_internal *)socket_address_storage;
	switch (address.family)
	{
	case AF_INET:
		address.addrlen = sizeof(sockaddr_in);
		break;
	case AF_INET6:
		address.addrlen = sizeof(sockaddr_in6);
		break;
	default:
		SDL_zerop(out_address);
		break;
	}

	if (address.family == AF_INET6)
	{
		// We do not accept ipv6 for now
		SDL_zerop(out_address);
		return;
	}

	copy_to_user_address(&address, out_address);
}

cnt_connection *cnt_connection_recv(cnt_connection *connection, cnt_stream *stream)
{
	SDL_assert(connection && "Connection is null pointer");
	SDL_assert(stream && "Stream is null pointer");

	cnt_sock_internal sock_internal;
	copy_to_internal_sock(&connection->src, &sock_internal);

	cnt_sockaddr_storage storage;
	cnt_socklen socket_length = sizeof(storage);

	int result = recvfrom(sock_internal.handle, (char *)stream->data, stream->capacity, 0, (sockaddr *)&storage, &socket_length);

	if (result == -1)
	{
		const int err = last_socket_error();
		if (would_block(err))
		{
			stream->at = 0;
		}
		return nullptr;
	}

	stream->at = result;

	query_address(&storage, &connection->dst);
	return connection;
}

void cnt_connection_close(cnt_connection *connection)
{
	SDL_assert(connection && "Connection is null pointer");

	SDL_zerop(connection);
}

static cnt_ip *cnt_star_find(cnt_star *star, cnt_ip *address)
{
	CNT_FOR(uint32_t, index, star->destinations.count)
	{
		cnt_ip *destination = star->destinations.addresses + index;
		if (cnt_ip_equals(destination, address))
		{
			return destination;
		}
	}
	return nullptr;
}

cnt_star *cnt_star_open(cnt_star *star, cnt_sock *sock, uint32_t max_connections)
{
	SDL_assert(star && "Star is null pointer");
	SDL_assert(sock && "Src is null pointer");

	SDL_memcpy(&star->sock, sock, sizeof(star->sock));

	cnt_ip_container_open(&star->destinations, max_connections);

	return star;
}

cnt_star *cnt_star_send(cnt_star *star, cnt_stream *stream)
{
	SDL_assert(star && "Star is null pointer");
	SDL_assert(stream && "Stream is null pointer");

	CNT_FOR(uint32_t, index, star->destinations.count)
	{
		cnt_ip *destination = star->destinations.addresses + index;

		cnt_connection connection;
		if (cnt_connection_open(&connection, &star->sock, destination))
		{
			cnt_connection_send(&connection, stream);
		}

		cnt_connection_close(&connection);
	}

	return star;
}

cnt_ip *cnt_star_recv(cnt_star *star, cnt_ip *address, cnt_stream *stream)
{
	SDL_assert(star && "Star is null pointer");
	SDL_assert(address && "Address is null pointer");
	SDL_assert(stream && "Stream is null pointer");

	cnt_connection connection;
	cnt_connection_from_socket(&connection, &star->sock);

	if (cnt_connection_recv(&connection, stream))
	{
		cnt_ip *addr = cnt_star_find(star, &connection.dst);
		if (addr == nullptr)
		{
			// Add new address if not found!
			cnt_ip_container_add(&star->destinations, &connection.dst);
		}

		SDL_memcpy(address, &connection.dst, sizeof(connection.dst));
		return address;
	}
	return nullptr;
}

cnt_star *cnt_star_add(cnt_star *star, cnt_ip *addr)
{
	SDL_assert(star && "Star is null pointer");
	SDL_assert(addr && "Addr is null pointer");

	SDL_assert(cnt_star_find(star, addr) != nullptr && "Addr is already added");

	cnt_ip_container_add(&star->destinations, addr);

	return star;
}

cnt_ip *cnt_star_remove(cnt_star *star, cnt_ip *addr)
{
	SDL_assert(star && "Star is null pointer");
	SDL_assert(addr && "Addr is null pointer");

	CNT_FOR(uint32_t, index, star->destinations.count)
	{
		cnt_ip *destination = star->destinations.addresses + index;
		if (cnt_ip_equals(destination, addr))
		{
			cnt_ip_container_remove_swap_at(&star->destinations, index);
			return addr;
		}
	}

	return nullptr;
}

void cnt_star_close(cnt_star *star)
{
	SDL_assert(star && "Star is null pointer");

	cnt_ip_container_close(&star->destinations);

	cnt_sock_close(&star->sock);

	SDL_zerop(star);
}

bool cnt_host_ip_try_find(cnt_host *host, cnt_ip *ip, uint32_t *at)
{
	// This is hackable
	for (uint32_t index = 0; index < (host->mapping.count); index++)
	{
		cnt_ip *address = &host->ip_lookup[index];
		if (cnt_ip_equals(address, ip))
		{
			*at = index;
			return true;
		}
	}
	return false;
}

cnt_protocol_client *cnt_protocol_client_create(cnt_protocol_client *client_protocol, cnt_client *client)
{
	SDL_assert(client && "Client is null pointer");
	SDL_assert(client_protocol && "Protocol is null pointer");
	SDL_assert(client->protocol != CNT_PROTOCOL_STATE_CLIENT_NONE && "Client is not allowed to be in NONE state");

	SDL_zerop(client_protocol);

	client_protocol->prefix = 8008;
	client_protocol->state = client->protocol;

	client->attempts = client->attempts + 1;

	switch (client_protocol->state)
	{
	// In any case, we send our secret over
	case CNT_PROTOCOL_STATE_CLIENT_REQUEST:
	case CNT_PROTOCOL_STATE_CLIENT_ANSWER:
	case CNT_PROTOCOL_STATE_CLIENT_OK: {
		cnt_stream stream;
		cnt_stream_create(&stream, client_protocol->extra_payload, sizeof(client_protocol->extra_payload));
		cnt_protocol_secret_write(&client->secret, &stream);
		client_protocol->extra_payload_count = stream.at;
		break;
	}
	}

	return client_protocol;
}

const uint16_t SECRET_SEED = 69;

cnt_client_on_host *cnt_protocol_client_apply(cnt_protocol_client *client_protocol, cnt_host *host, cnt_ip *client_addr)
{
	SDL_assert(host && "Host is null pointer");
	SDL_assert(client_protocol && "Protocol is null pointer");

	bool should_drop = client_protocol->prefix != 8008 || client_protocol->state == CNT_PROTOCOL_STATE_CLIENT_NONE;
	if (should_drop)
	{
		return nullptr;
	}

	cnt_dense_index dense_index;
	bool client_id_exists = cnt_sparse_list_try_get_dense(&host->mapping, &client_protocol->id, &dense_index);
	if (client_protocol->state == CNT_PROTOCOL_STATE_CLIENT_OK)
	{
		// Any kind of client can just send as an OK
		// When a client sends an OK, we just verify a few data entries
		if (client_id_exists)
		{
			cnt_ip *stored_addr = &host->ip_lookup[dense_index.index];
			bool is_stored_addr_mapped_correctly = cnt_ip_equals(stored_addr, client_addr);
			if (is_stored_addr_mapped_correctly)
			{
				cnt_stream stream;
				cnt_stream_create(&stream, client_protocol->extra_payload, client_protocol->extra_payload_count);

				cnt_client_on_host *client_state = &host->client_states[dense_index.index];
				cnt_protocol_secret_read(&client_state->secret, &stream);
				bool is_secret_correct = (client_state->secret.public_value ^ client_state->secret.private_value) == SECRET_SEED;
				if (is_secret_correct)
				{
					client_state->attempts = 0;
					return client_state;
				}
			}
		}
		// Client sent us bad data - we cannot change server state just because some client is a cunt
		SDL_Log("Received naughty state from client on Host - Tried to circumvent handshake?");
		return nullptr;
	}

	if (!client_id_exists)
	{
		if (!cnt_host_ip_try_find(host, client_addr, &dense_index.index))
		{
			cnt_sparse_index id;
			if (!cnt_sparse_list_create(&host->mapping, &id))
			{
				SDL_Log("Host is full - Cannot take in client. Bummer :(");
				return nullptr;
			}
			bool has_id = cnt_sparse_list_try_get_dense(&host->mapping, &id, &dense_index);
			SDL_assert(has_id);

			SDL_memcpy(host->ip_lookup + dense_index.index, client_addr, sizeof(*host->ip_lookup));

			cnt_client_on_host *client_state = &host->client_states[dense_index.index];
			SDL_zerop(client_state);
			client_state->id = id;
		}
		else
		{
			// Internal fuck ups
			cnt_sparse_index id;
			bool has_sparse_index = cnt_sparse_list_try_get_sparse(&host->mapping, &dense_index, &id);
			SDL_assert(has_sparse_index && "Address exists in lookup, but did not get shoved into lookup");

			cnt_client_on_host *client_state = &host->client_states[dense_index.index];
			bool is_stored_id_correct = client_state->id.index == id.index;
			SDL_assert(is_stored_id_correct && "Client-Address mapping is completely fucked up");
		}
	}

	cnt_ip *stored_addr = &host->ip_lookup[dense_index.index];
	bool is_stored_addr_empty = cnt_ip_is_empty(stored_addr);
	if (is_stored_addr_empty)
	{
		SDL_memcpy(stored_addr, client_addr, sizeof(*stored_addr));
	}
	else
	{
		bool is_stored_addr_mapped_correctly = cnt_ip_equals(stored_addr, client_addr);
		if (!is_stored_addr_mapped_correctly)
		{
			// This can actually happen if the client (dumb) sends us 0.
			// Maybe we should reject with a reason, telling the client that its protocol initalisation failed
			SDL_Log("Received naughty state from client on Host - Client impersonates?");
			return nullptr;
		}
	}

	cnt_client_on_host *client_state = &host->client_states[dense_index.index];

	// Whenever we receive something, we can reset the attempts!
	client_state->attempts = 0;

	// TODO: This breaks stuff....
	if (client_state->protocol >= client_protocol->state)
	{
		return nullptr;
	}

	if (client_protocol->state == CNT_PROTOCOL_STATE_CLIENT_REQUEST)
	{
		client_state->secret.private_value = 420;
		client_state->secret.public_value = SECRET_SEED ^ client_state->secret.private_value;
		client_state->secret.state = CNT_SECRET_STATE_OUTDATED;
		client_state->protocol = CNT_PROTOCOL_STATE_HOST_CHALLENGE;
		SDL_Log("Client (%lu) requests to join\nSend Challenge to Client (%lu)", client_state->id.index, client_state->id.index);

		return nullptr;
	}

	if (client_protocol->state == CNT_PROTOCOL_STATE_CLIENT_ANSWER)
	{
		cnt_stream stream;
		cnt_stream_create(&stream, client_protocol->extra_payload, client_protocol->extra_payload_count);
		cnt_protocol_secret_read(&client_state->secret, &stream);
		uint16_t client_value = SECRET_SEED ^ client_state->secret.public_value;
		bool is_correct = client_state->secret.private_value == client_value;

		client_state->secret.state = is_correct ? CNT_SECRET_STATE_ACCEPTED : CNT_SECRET_STATE_REJECTED;
		client_state->protocol = is_correct ? CNT_PROTOCOL_STATE_HOST_OK : CNT_PROTOCOL_STATE_HOST_RESOLUTION_REJECT;
		SDL_Log("Client (%lu) answered to challenge\nSend response to Client (%lu) - %s", client_state->id.index, client_state->id.index,
		        is_correct ? "Accepted" : "Rejected");

		return nullptr;
	}

	return nullptr;
}

cnt_client *cnt_protocol_host_apply(cnt_protocol_host *host_protocol, cnt_client *client)
{
	SDL_assert(client && "Client is null pointer");
	SDL_assert(host_protocol && "Protocol is null pointer");

	bool should_drop = host_protocol->prefix != 8008 || host_protocol->state == CNT_PROTOCOL_STATE_HOST_NONE ||
	                   client->protocol == CNT_PROTOCOL_STATE_CLIENT_NONE;
	if (should_drop)
	{
		return nullptr;
	}

	client->host_protocol = host_protocol->state;
	client->attempts = 0;

	switch (host_protocol->state)
	{
	case CNT_PROTOCOL_STATE_HOST_CHALLENGE: {
		cnt_stream stream;
		cnt_stream_create(&stream, host_protocol->extra_payload, host_protocol->extra_payload_count);
		cnt_stream_read_uint32(&stream, &client->id_on_host.index);
		cnt_protocol_secret_read(&client->secret, &stream);
		if (client->secret.state != CNT_SECRET_STATE_OUTDATED)
		{
			break;
		}
		client->secret.public_value = client->secret.public_value;

		SDL_Log("Client (%lu) received challenge\nSend Answer to Host", client->id_on_host.index);

		client->protocol = CNT_PROTOCOL_STATE_CLIENT_ANSWER;
		return nullptr;
	}
	case CNT_PROTOCOL_STATE_HOST_RESOLUTION_REJECT:
	case CNT_PROTOCOL_STATE_HOST_KICKED:
		// Umm...
		return nullptr;
	case CNT_PROTOCOL_STATE_HOST_OK:
		client->protocol = CNT_PROTOCOL_STATE_CLIENT_OK;
		break;
	}
	return client;
}

cnt_protocol_host *cnt_protocol_host_create_ok(cnt_protocol_host *host_protocol)
{
	CNT_NULL_CHECK(host_protocol);

	SDL_zerop(host_protocol);

	host_protocol->prefix = 8008;
	host_protocol->state = CNT_PROTOCOL_STATE_HOST_OK;
	return host_protocol;
}

cnt_protocol_host *cnt_protocol_host_create(cnt_protocol_host *host_protocol, cnt_host *host, cnt_client_on_host *client_state)
{
	CNT_NULL_CHECK(host);
	CNT_NULL_CHECK(client_state);
	CNT_NULL_CHECK(host_protocol);

	SDL_assert(client_state->protocol != CNT_PROTOCOL_STATE_HOST_NONE && "Client state is in invalid state. It shall not be NONE");

	SDL_zerop(host_protocol);

	host_protocol->prefix = 8008;
	host_protocol->state = client_state->protocol;

	if (host_protocol->state == CNT_PROTOCOL_STATE_HOST_OK)
	{
		// Host does not send header. Take it or leave it LOL
		// ... Maybe send a thin header to understand we are part of that layer?
		return host_protocol;
	}

	if (host_protocol->state == CNT_PROTOCOL_STATE_HOST_CHALLENGE)
	{
		cnt_stream stream;
		cnt_stream_create(&stream, host_protocol->extra_payload, sizeof(host_protocol->extra_payload));
		cnt_stream_write_uint32(&stream, client_state->id.index);
		cnt_protocol_secret_write(&client_state->secret, &stream);
		host_protocol->extra_payload_count = stream.at;
	}

	return host_protocol;
}

cnt_host *cnt_host_open(cnt_host *host, uint32_t max_connections)
{
	SDL_assert(host && "Host is null pointer");

	cnt_sparse_list_open(&host->mapping, max_connections);

	host->client_states = (cnt_client_on_host *)SDL_malloc(sizeof(*host->client_states) * max_connections);
	host->ip_lookup = (cnt_ip *)SDL_malloc(sizeof(*host->ip_lookup) * max_connections);

	return host;
}

bool cnt_host_is_client_connected(cnt_host *host, cnt_sparse_index client_id)
{
	SDL_assert(host && "Host is null pointer");

	cnt_dense_index dense_index;
	if (!cnt_sparse_list_try_get_dense(&host->mapping, &client_id, &dense_index))
	{
		return false;
	}
	cnt_client_on_host *client_on_host = &host->client_states[dense_index.index];
	return client_on_host->protocol == CNT_PROTOCOL_STATE_HOST_OK;
}

bool cnt_host_client_ip_try_get(cnt_host *host, cnt_sparse_index client_id, cnt_ip *ip)
{
	SDL_assert(host && "Host is null pointer");
	SDL_assert(ip && "IP is null pointer");

	cnt_dense_index dense_index;
	if (!cnt_sparse_list_try_get_dense(&host->mapping, &client_id, &dense_index))
	{
		return false;
	}
	SDL_memcpy(ip, &host->ip_lookup[dense_index.index], sizeof(*ip));
	return true;
}

cnt_host *cnt_host_send(cnt_host *host, cnt_stream *stream, cnt_ip_container *container, cnt_message_64_bytes_queue *messages)
{
	SDL_assert(host && "Host is null pointer");

	host->send_count = host->send_count + 1;

	// All ok clients get a thin host protocol telling them everything is fine
	cnt_protocol_host host_protocol;
	cnt_protocol_host_create_ok(&host_protocol);
	cnt_protocol_host_write(&host_protocol, stream);

	// TODO: Make config
	const uint8_t MAXIMUM_HANDSHAKE_ATTEMPTS = 64;

	// The container declares who we send shit to
	cnt_ip_container_clear(container);

	// Clean up clients stuck in handshake or clients with a faulty protocol
	cnt_sparse_list *mapping = &host->mapping;
	for (uint32_t index = 0; index < mapping->count; index++)
	{
		cnt_dense_index dense_index = mapping->dense[index];
		cnt_client_on_host *client_state = &host->client_states[dense_index.index];

		cnt_protocol_host protocol;
		// NOTE: Maybe there are better conditions to check this...
		if (client_state->protocol == CNT_PROTOCOL_STATE_HOST_NONE || client_state->attempts > MAXIMUM_HANDSHAKE_ATTEMPTS)
		{
			// We get the last index BEFORE we remove.
			// Only for consistency so last is count - 1
			uint32_t last = mapping->count - 1;

			cnt_dense_index dirty_dense;
			bool has_dense = cnt_sparse_list_try_get_dense(mapping, &client_state->id, &dirty_dense);
			SDL_assert(has_dense && dirty_dense.index == dense_index.index && "Dense index does not exist for sparse index");

			bool remove_result = cnt_sparse_list_remove(mapping, &client_state->id);
			SDL_assert(remove_result && "Removal of connection id was unsuccessful");

			// If the asserts above fail we are in a critical state :( Could create a bruteforce approach in the long run to recover
			SDL_memcpy(&host->ip_lookup[dirty_dense.index], &host->ip_lookup[last], sizeof(*host->ip_lookup));
			SDL_memcpy(&host->client_states[dirty_dense.index], &host->client_states[last], sizeof(*host->client_states));

			// Null out last
			SDL_zerop(host->ip_lookup + last);
			SDL_zerop(host->client_states + last);

			// We want to repeat index since we removed 1 from count and moved last into current
			index = index - 1;
		}
	}

	for (uint32_t index = 0; index < mapping->count; index++)
	{
		cnt_dense_index dense_index = mapping->dense[index];
		cnt_client_on_host *client_state = &host->client_states[dense_index.index];

		cnt_ip *ip = &host->ip_lookup[dense_index.index];

		cnt_protocol_host protocol;
		if (cnt_protocol_host_create(&protocol, host, client_state))
		{
			if (protocol.state == CNT_PROTOCOL_STATE_HOST_OK)
			{
				cnt_ip_container_add(container, ip);
			}
			else
			{
				cnt_message_64_bytes message;
				SDL_memcpy(&message.ip, ip, sizeof(message.ip));

				cnt_stream protocol_stream;
				cnt_stream_create(&protocol_stream, message.payload, sizeof(message.payload));

				cnt_protocol_host_write(&protocol, &protocol_stream);
				message.payload_count = protocol_stream.at;
				cnt_message_queue_64_bytes_push(messages, &message);
			}
		}

		// Whenever we send something to a client, we increment the attempts by 1
		// Whenever we recv something, we set the client's attempt counter to 0
		// We do this cause we want to be indipendent from a frequency.
		// We want to make sure that we stay within a margin of error.
		// If we allow 100 attempts, and we consistently reach a attempt count of
		// 75, we can confidently 75% of the packets are getting lost.
		// We can further more optimise this approach by calculating a frequency requirement
		// on top of the attempts counter, so attempts 0 to 10 map to 10ms and 90 to 100 map to 250ms
		client_state->attempts = client_state->attempts + 1;
	}

	return host;
}

cnt_client_on_host *cnt_host_recv(cnt_host *host, cnt_ip *client_addr, cnt_stream *stream)
{
	SDL_assert(host && "Host is null pointer");

	host->recv_count = host->recv_count + 1;

	cnt_protocol_client protocol;
	cnt_protocol_client_read(&protocol, stream);

	return cnt_protocol_client_apply(&protocol, host, client_addr);
}

bool cnt_host_kick(cnt_host *host, cnt_sparse_index client_id)
{
	SDL_assert(host && "Host is null pointer");

	cnt_dense_index dense_index;
	if (cnt_sparse_list_try_get_dense(&host->mapping, &client_id, &dense_index))
	{
		cnt_client_on_host *state = &host->client_states[dense_index.index];
		state->protocol = CNT_PROTOCOL_STATE_HOST_KICKED;
		return true;
	}

	return false;
}

void cnt_host_close(cnt_host *host)
{
	SDL_assert(host && "Host is null pointer");

	SDL_free(host->ip_lookup);
	SDL_free(host->client_states);
	cnt_sparse_list_close(&host->mapping);

	SDL_zerop(host);
}

cnt_client *cnt_client_open(cnt_client *client, cnt_connection *connection)
{
	SDL_assert(client && "Client is null pointer");
	SDL_assert(connection && "Connection is null pointer");

	SDL_zerop(client);

	cnt_connection_open(&client->connection, &connection->src, &connection->dst);
	client->id_on_host.index = UINT32_MAX;
	client->protocol = CNT_PROTOCOL_STATE_CLIENT_REQUEST;

	return client;
}

cnt_client *cnt_client_send(cnt_client *client, cnt_stream *stream)
{
	CNT_NULL_CHECK(client);
	CNT_NULL_CHECK(stream);

	const uint8_t MAXIMUM_HANDSHAKE_ATTEMPTS = 64;

	if (client->protocol == CNT_PROTOCOL_STATE_CLIENT_NONE || client->attempts > MAXIMUM_HANDSHAKE_ATTEMPTS)
	{
		// Timeout or invalid state - Nothing can be done
		return nullptr;
	}

	// Preferably prefer a message!
	cnt_protocol_client protocol;
	if (!cnt_protocol_client_create(&protocol, client))
	{
		// Failed to follow protocol - Nothing can be done
		return nullptr;
	}

	if (client->host_protocol > CNT_PROTOCOL_STATE_HOST_OK)
	{
		// Host booted us :(
		return nullptr;
	}
	cnt_protocol_client_write(&protocol, stream);

	if (client->protocol != CNT_PROTOCOL_STATE_CLIENT_OK)
	{
		// Work on handshake
		return nullptr;
	}

	// Normal flow
	return client;
}

cnt_client *cnt_client_recv(cnt_client *client, cnt_stream *stream)
{
	CNT_NULL_CHECK(client);
	CNT_NULL_CHECK(stream);

	cnt_protocol_host protocol;
	cnt_protocol_host_read(&protocol, stream);
	return cnt_protocol_host_apply(&protocol, client);
}

void cnt_client_close(cnt_client *client)
{
	SDL_assert(client && "Client is null pointer");

	cnt_connection_close(&client->connection);

	SDL_zerop(client);
}

cnt_user_client_frame *cnt_user_client_frame_alloc(cnt_stream *stream)
{
	CNT_NULL_CHECK(stream);

	// Heap allocation for this one is very stupid
	const uint32_t size = offsetof(cnt_user_client_frame, data[stream->at]);
	cnt_user_client_frame *frame = (cnt_user_client_frame *)SDL_malloc(size);
	frame->count = stream->at;

	cnt_stream frame_stream;
	cnt_stream_create(&frame_stream, frame->data, frame->count);
	cnt_stream_write_string(&frame_stream, stream->data, stream->at);
	return frame;
}

void cnt_user_client_frame_free(cnt_user_client_frame *frame)
{
	CNT_NULL_CHECK(frame);
	SDL_free(frame);
}

cnt_queue_header *cnt_queue_header_open(cnt_queue_header *header, uint32_t capacity)
{
	CNT_NULL_CHECK(header);
	header->capacity = capacity;
	header->head = 0;
	header->tail = 0;
	return header;
}

uint32_t cnt_queue_header_count(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	return (header->tail - header->head + header->capacity) % header->capacity;
}

void cnt_queue_header_assert_is_not_full(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	// TODO: This fails :D 
	// WE just overwrite in this case!!!!!! LOL 

	uint32_t count = cnt_queue_header_count(header);
	(void)count;

	SDL_assert(count < header->capacity);
}

bool cnt_queue_header_is_empty(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	return header->head == header->tail;
}

uint32_t cnt_queue_header_head(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	return header->head;
}

uint32_t cnt_queue_header_tail(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	return header->tail;
}

uint32_t cnt_queue_header_advance_head(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	uint32_t index = cnt_queue_header_head(header);

	header->head = (header->head + 1) % header->capacity;
	return index;
}

uint32_t cnt_queue_header_advance_tail(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	uint32_t index = cnt_queue_header_tail(header);

	header->tail = (header->tail + 1) % header->capacity;
	return index;
}

void cnt_queue_header_clear(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);
	header->head = 0;
	header->tail = 0;
}

void cnt_queue_header_close(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	SDL_zerop(header);
}

bool cnt_queue_header_try_enqueue(cnt_queue_header *header, void *data, size_t stride, void *element)
{
	CNT_NULL_CHECK(header);

	// try_enqueue false not implemented

	cnt_queue_header_assert_is_not_full(header);

	uint32_t tail = cnt_queue_header_advance_tail(header);
	SDL_memcpy(((uint8_t *)data) + tail * stride, element, stride);

	return true;
}

bool cnt_queue_header_try_peek(cnt_queue_header *header, void *data, size_t stride, void *element)
{
	CNT_NULL_CHECK(header);

	if (cnt_queue_header_is_empty(header))
	{
		return false;
	}

	uint32_t head = cnt_queue_header_head(header);
	uint8_t *data_at = ((uint8_t *)data) + head * stride;
	SDL_memcpy(element, data_at, stride);

	return true;
}

bool cnt_queue_header_try_dequeue(cnt_queue_header *header, void *data, size_t stride, void *element)
{
	CNT_NULL_CHECK(header);

	if (cnt_queue_header_is_empty(header))
	{
		return false;
	}

	uint32_t head = cnt_queue_header_advance_head(header);
	uint8_t *data_at = ((uint8_t *)data) + head * stride;
	SDL_memcpy(element, data_at, stride);
	SDL_memset(data_at, 0, stride);

	return true;
}

#define CNT_DEFINE_QUEUE(type, element_type, data_name)                                                                                    \
	type *type##_alloc(uint32_t capacity)                                                                                                  \
	{                                                                                                                                      \
		const uint32_t size = offsetof(type, data_name[capacity]);                                                                         \
		type *queue = (type *)SDL_malloc(size);                                                                                            \
		cnt_queue_header_open(&queue->header, capacity);                                                                                   \
		return queue;                                                                                                                      \
	}                                                                                                                                      \
                                                                                                                                           \
	void type##_free(type *queue)                                                                                                          \
	{                                                                                                                                      \
		CNT_NULL_CHECK(queue);                                                                                                             \
		cnt_queue_header_close(&queue->header);                                                                                            \
		SDL_free(queue);                                                                                                                   \
	}                                                                                                                                      \
                                                                                                                                           \
	uint32_t type##_count(type *queue)                                                                                                     \
	{                                                                                                                                      \
		CNT_NULL_CHECK(queue);                                                                                                             \
                                                                                                                                           \
		return cnt_queue_header_count(&queue->header);                                                                                     \
	}                                                                                                                                      \
                                                                                                                                           \
	void type##_clear(type *queue)                                                                                                         \
	{                                                                                                                                      \
		CNT_NULL_CHECK(queue);                                                                                                             \
                                                                                                                                           \
		cnt_queue_header_clear(&queue->header);                                                                                            \
	}                                                                                                                                      \
                                                                                                                                           \
	void type##_enqueue(type *queue, element_type data)                                                                                    \
	{                                                                                                                                      \
		CNT_NULL_CHECK(queue);                                                                                                             \
                                                                                                                                           \
		cnt_queue_header_try_enqueue(&queue->header, (void *)queue->data_name, sizeof(*queue->data_name), (void *)&data);                  \
	}                                                                                                                                      \
                                                                                                                                           \
	bool type##_is_empty(type *queue)                                                                                                      \
	{                                                                                                                                      \
		CNT_NULL_CHECK(queue);                                                                                                             \
                                                                                                                                           \
		return cnt_queue_header_is_empty(&queue->header);                                                                                  \
	}                                                                                                                                      \
                                                                                                                                           \
	bool type##_try_peek(type *queue, element_type *data)                                                                                  \
	{                                                                                                                                      \
		CNT_NULL_CHECK(queue);                                                                                                             \
		CNT_NULL_CHECK(data);                                                                                                              \
                                                                                                                                           \
		return cnt_queue_header_try_peek(&queue->header, (void *)queue->data_name, sizeof(*queue->data_name), (void *)data);               \
	}                                                                                                                                      \
                                                                                                                                           \
	bool type##_try_dequeue(type *queue, element_type *data)                                                                               \
	{                                                                                                                                      \
		CNT_NULL_CHECK(queue);                                                                                                             \
		CNT_NULL_CHECK(data);                                                                                                              \
                                                                                                                                           \
		return cnt_queue_header_try_dequeue(&queue->header, (void *)queue->data_name, sizeof(*queue->data_name), (void *)data);            \
	}

CNT_DEFINE_QUEUE(cnt_user_client_command_queue, cnt_user_client_command, commands)
CNT_DEFINE_QUEUE(cnt_user_client_frame_queue, cnt_user_client_frame *, frames)
CNT_DEFINE_QUEUE(cnt_user_host_command_queue, cnt_user_host_command, commands)
CNT_DEFINE_QUEUE(cnt_user_host_frame_queue, cnt_user_host_frame *, frames)

cnt_user_client_frame_concurrent_queue *cnt_user_client_frame_concurrent_queue_open(cnt_user_client_frame_concurrent_queue *queue,
                                                                                    uint32_t capacity)
{
	CNT_NULL_CHECK(queue);

	SDL_zerop(queue);

	queue->queues[0] = cnt_user_client_frame_queue_alloc(capacity);
	queue->queues[1] = cnt_user_client_frame_queue_alloc(capacity);

	return queue;
}

void cnt_user_client_frame_concurrent_queue_close(cnt_user_client_frame_concurrent_queue *queue)
{
	CNT_NULL_CHECK(queue);

	cnt_user_client_frame_queue_free(queue->queues[0]);
	cnt_user_client_frame_queue_free(queue->queues[1]);

	SDL_zerop(queue);
}

cnt_user_client_frame_concurrent_queue *cnt_user_client_frame_concurrent_queue_enqueue(cnt_user_client_frame_concurrent_queue *queue,
                                                                                       cnt_user_client_frame *frame)
{
	CNT_NULL_CHECK(queue);

	cnt_user_client_frame_queue *current_queue = queue->queues[queue->current_inactive];
	cnt_user_client_frame_queue_enqueue(current_queue, frame);

	return queue;
}

cnt_user_client_frame_concurrent_queue *cnt_user_client_frame_concurrent_queue_submit(cnt_user_client_frame_concurrent_queue *queue)
{
	CNT_NULL_CHECK(queue);

	// While we have an active queue, it implies the user is still working on previosu data
	cnt_user_client_frame_queue *active = (cnt_user_client_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == nullptr)
	{
		cnt_user_client_frame_queue *current_queue = queue->queues[queue->current_inactive];
		SDL_SetAtomicPointer((void **)&queue->active, current_queue);

		// flip flop between the inactive queues when user does not have any
		queue->current_inactive = (queue->current_inactive + 1) % 2;
	}

	return queue;
}

bool cnt_user_client_frame_concurrent_queue_try_dequeue(cnt_user_client_frame_concurrent_queue *queue, cnt_user_client_frame **frame)
{
	CNT_NULL_CHECK(queue);
	CNT_NULL_CHECK(frame);

	cnt_user_client_frame_queue *active = (cnt_user_client_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == nullptr)
	{
		return false;
	}

	if (cnt_user_client_frame_queue_try_dequeue(active, frame))
	{
		if (cnt_user_client_frame_queue_is_empty(active))
		{
			SDL_SetAtomicPointer((void **)&queue->active, nullptr);
		}
		return true;
	}

	return false;
}

bool cnt_user_client_frame_concurrent_queue_try_peek(cnt_user_client_frame_concurrent_queue *queue, cnt_user_client_frame **frame)
{
	CNT_NULL_CHECK(queue);
	CNT_NULL_CHECK(frame);

	cnt_user_client_frame_queue *active = (cnt_user_client_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == nullptr)
	{
		return false;
	}

	if (cnt_user_client_frame_queue_try_peek(active, frame))
	{
		return true;
	}

	return false;
}

cnt_user_host_frame *cnt_user_host_frame_alloc(cnt_stream *stream, cnt_sparse_index client_id)
{
	CNT_NULL_CHECK(stream);

	// Heap allocation for this one is very stupid
	const uint32_t size = offsetof(cnt_user_host_frame, data[stream->at]);
	cnt_user_host_frame *frame = (cnt_user_host_frame *)SDL_malloc(size);
	frame->count = stream->at;
	frame->client_id = client_id;

	cnt_stream frame_stream;
	cnt_stream_create(&frame_stream, frame->data, frame->count);
	cnt_stream_write_string(&frame_stream, stream->data, stream->at);
	return frame;
}

void cnt_user_host_frame_free(cnt_user_host_frame *frame)
{
	CNT_NULL_CHECK(frame);
	SDL_free(frame);
}

cnt_user_host_frame_concurrent_queue *cnt_user_host_frame_concurrent_queue_open(cnt_user_host_frame_concurrent_queue *queue,
                                                                                uint32_t capacity)
{
	CNT_NULL_CHECK(queue);

	SDL_zerop(queue);

	queue->queues[0] = cnt_user_host_frame_queue_alloc(capacity);
	queue->queues[1] = cnt_user_host_frame_queue_alloc(capacity);

	return queue;
}

void cnt_user_host_frame_concurrent_queue_close(cnt_user_host_frame_concurrent_queue *queue)
{
	CNT_NULL_CHECK(queue);

	cnt_user_host_frame_queue_free(queue->queues[0]);
	cnt_user_host_frame_queue_free(queue->queues[1]);

	SDL_zerop(queue);
}

cnt_user_host_frame_concurrent_queue *cnt_user_host_frame_concurrent_queue_enqueue(cnt_user_host_frame_concurrent_queue *queue,
                                                                                   cnt_user_host_frame *frame)
{
	CNT_NULL_CHECK(queue);

	cnt_user_host_frame_queue *current_queue = queue->queues[queue->current_inactive];
	cnt_user_host_frame_queue_enqueue(current_queue, frame);

	return queue;
}

// Maybe commit to keep it cool
cnt_user_host_frame_concurrent_queue *cnt_user_host_frame_concurrent_queue_submit(cnt_user_host_frame_concurrent_queue *queue)
{
	CNT_NULL_CHECK(queue);

	// While we have an active queue, it implies the user is still working on previosu data
	cnt_user_host_frame_queue *active = (cnt_user_host_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == nullptr)
	{
		cnt_user_host_frame_queue *current_queue = queue->queues[queue->current_inactive];
		SDL_SetAtomicPointer((void **)&queue->active, current_queue);

		// flip flop between the inactive queues when user does not have any
		queue->current_inactive = (queue->current_inactive + 1) % 2;
	}

	return queue;
}

bool cnt_user_host_frame_concurrent_queue_try_dequeue(cnt_user_host_frame_concurrent_queue *queue, cnt_user_host_frame **frame)
{
	CNT_NULL_CHECK(queue);
	CNT_NULL_CHECK(frame);

	cnt_user_host_frame_queue *active = (cnt_user_host_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == nullptr)
	{
		return false;
	}

	if (cnt_user_host_frame_queue_try_dequeue(active, frame))
	{
		if (cnt_user_host_frame_queue_is_empty(active))
		{
			SDL_SetAtomicPointer((void **)&queue->active, nullptr);
		}
		return true;
	}

	return false;
}

bool cnt_user_host_frame_concurrent_queue_try_peek(cnt_user_host_frame_concurrent_queue *queue, cnt_user_host_frame **frame)
{
	CNT_NULL_CHECK(queue);
	CNT_NULL_CHECK(frame);

	cnt_user_host_frame_queue *active = (cnt_user_host_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == nullptr)
	{
		return false;
	}

	if (cnt_user_host_frame_queue_try_peek(active, frame))
	{
		return true;
	}

	return false;
}

cnt_user_host_command_concurrent_queue *cnt_user_host_command_concurrent_queue_open(cnt_user_host_command_concurrent_queue *queue,
                                                                                    uint32_t capacity)
{
	CNT_NULL_CHECK(queue);

	SDL_zerop(queue);

	queue->queues[0] = cnt_user_host_command_queue_alloc(capacity);
	queue->queues[1] = cnt_user_host_command_queue_alloc(capacity);

	return queue;
}

void cnt_user_host_command_concurrent_queue_close(cnt_user_host_command_concurrent_queue *queue, uint32_t capacity)
{
	CNT_NULL_CHECK(queue);

	cnt_user_host_command_queue_free(queue->queues[0]);
	cnt_user_host_command_queue_free(queue->queues[1]);

	SDL_zerop(queue);
}

cnt_user_host_command_concurrent_queue *cnt_user_host_command_concurrent_queue_enqueue(cnt_user_host_command_concurrent_queue *queue,
                                                                                       cnt_user_host_command *command)
{
	CNT_NULL_CHECK(queue);

	cnt_user_host_command_queue *current_queue = queue->queues[queue->current_inactive];

	// Not optimal... Maybe the generic macro for a queue was a bad idea: Change it!
	cnt_user_host_command_queue_enqueue(current_queue, *command);

	return queue;
}

cnt_user_host_command_concurrent_queue *cnt_user_host_command_concurrent_queue_submit(cnt_user_host_command_concurrent_queue *queue)
{
	CNT_NULL_CHECK(queue);

	// While we have an active queue, it implies the user is still working on previosu data
	cnt_user_host_frame_queue *active = (cnt_user_host_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == nullptr)
	{
		cnt_user_host_command_queue *current_queue = queue->queues[queue->current_inactive];
		SDL_SetAtomicPointer((void **)&queue->active, current_queue);

		// flip flop between the inactive queues when user does not have any
		queue->current_inactive = (queue->current_inactive + 1) % 2;
	}

	return queue;
}

bool cnt_user_host_command_concurrent_queue_try_dequeue(cnt_user_host_command_concurrent_queue *queue, cnt_user_host_command *command)
{
	CNT_NULL_CHECK(queue);
	CNT_NULL_CHECK(command);

	cnt_user_host_command_queue *active = (cnt_user_host_command_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == nullptr)
	{
		return false;
	}

	if (cnt_user_host_command_queue_try_dequeue(active, command))
	{
		if (cnt_user_host_command_queue_is_empty(active))
		{
			SDL_SetAtomicPointer((void **)&queue->active, nullptr);
		}
		return true;
	}

	return false;
}

cnt_user_client *cnt_user_client_create(cnt_user_client *user, const char *host_ip, uint16_t host_port)
{
	CNT_NULL_CHECK(user);
	CNT_NULL_CHECK(host_ip);
	SDL_assert(host_port != 0 && "Host port - impossible to connect to");

	// Maybe copy IP in since this will not work with matchmakers, in other words, it will be unsafe
	user->host_ip = host_ip;
	user->host_port = host_port;

	cnt_user_client_frame_concurrent_queue_open(&user->send_queue, 64);
	cnt_user_client_frame_concurrent_queue_open(&user->recv_queue, 64);

	SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)cnt_client_default_process, "", user);
	SDL_DetachThread(thread);

	return user;
}

cnt_user_host *cnt_user_host_create(cnt_user_host *user, const char *host_ip, uint16_t host_port)
{
	CNT_NULL_CHECK(user);
	CNT_NULL_CHECK(host_ip);

	// Maybe copy IP in since this will not work with matchmakers
	user->host_ip = host_ip;
	user->host_port = host_port;

	cnt_user_host_frame_concurrent_queue_open(&user->send_queue, 64);
	cnt_user_host_frame_concurrent_queue_open(&user->recv_queue, 64);

	cnt_user_host_command_concurrent_queue_open(&user->command_queue, 64);

	SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)cnt_host_default_process, "", user);
	SDL_DetachThread(thread);

	return user;
}

cnt_user_client *cnt_user_client_send(cnt_user_client *client, void *ptr, int byte_count)
{
	cnt_stream example_stream;
	cnt_stream_create_full(&example_stream, (uint8_t *)ptr, byte_count);
	cnt_user_client_frame *frame = cnt_user_client_frame_alloc(&example_stream);
	cnt_user_client_frame_concurrent_queue_enqueue(&client->send_queue, frame);
	cnt_user_client_frame_concurrent_queue_submit(&client->send_queue);
	return client;
}

int cnt_user_client_recv(cnt_user_client *client, void *ptr, int byte_count)
{
	cnt_user_client_frame *frame;
	if (!cnt_user_client_frame_concurrent_queue_try_peek(&client->recv_queue, &frame))
	{
		return 0;
	}
	if (frame->count > byte_count)
	{
		// Might make sense to give an entry point to read a partial frame
		return -1;
	}
	bool has_frame = cnt_user_client_frame_concurrent_queue_try_dequeue(&client->recv_queue, &frame);
	SDL_assert(has_frame);

	SDL_memcpy(ptr, frame->data, frame->count);

	int result = frame->count;
	cnt_user_client_frame_free(frame);

	return result;
}

cnt_user_host *cnt_user_host_broadcast(cnt_user_host *host, void *ptr, int byte_count)
{
	CNT_NULL_CHECK(host);
	CNT_NULL_CHECK(ptr);

	cnt_stream example_stream;
	cnt_stream_create_full(&example_stream, (uint8_t *)ptr, byte_count);
	// TODO: Fix frame having redundant field - make host frame and client frame separate concepts!
	cnt_user_host_frame *frame = cnt_user_host_frame_alloc(&example_stream, {UINT32_MAX});
	cnt_user_host_frame_concurrent_queue_enqueue(&host->send_queue, frame);
	cnt_user_host_frame_concurrent_queue_submit(&host->send_queue);
	return host;
}

cnt_user_host *cnt_user_host_send(cnt_user_host *host, cnt_sparse_index client_id, void *ptr, int byte_count)
{
	CNT_NULL_CHECK(host);
	CNT_NULL_CHECK(ptr);
	
	cnt_stream example_stream;
	cnt_stream_create_full(&example_stream, (uint8_t *)ptr, byte_count);
	// TODO: Fix frame having redundant field - make host frame and client frame separate concepts!
	cnt_user_host_frame *frame = cnt_user_host_frame_alloc(&example_stream, client_id);
	cnt_user_host_frame_concurrent_queue_enqueue(&host->send_queue, frame);
	cnt_user_host_frame_concurrent_queue_submit(&host->send_queue);
	return host;
}

cnt_user_host *cnt_user_host_kick(cnt_user_host *host, cnt_sparse_index client_id)
{
	CNT_NULL_CHECK(host);

	cnt_user_host_command command;
	command.type = CNT_USER_HOST_COMMAND_TYPE_KICK;
	command.kick.client = client_id;
	cnt_user_host_command_concurrent_queue_enqueue(&host->command_queue, &command);
	cnt_user_host_command_concurrent_queue_submit(&host->command_queue);
	return host;
}

int cnt_user_host_recv(cnt_user_host *host, cnt_sparse_index *client_id, void *ptr, int byte_count)
{
	cnt_user_host_frame *frame;
	if (!cnt_user_host_frame_concurrent_queue_try_peek(&host->recv_queue, &frame))
	{
		return 0;
	}
	if (frame->count > byte_count)
	{
		// Might make sense to give an entry point to read a partial frame
		return -1;
	}
	bool has_frame = cnt_user_host_frame_concurrent_queue_try_dequeue(&host->recv_queue, &frame);
	SDL_assert(has_frame);

	SDL_memcpy(client_id, &frame->client_id, sizeof(*client_id));
	SDL_memcpy(ptr, frame->data, frame->count);

	int result = frame->count;
	cnt_user_host_frame_free(frame);

	return result;
}

int cnt_client_default_process(cnt_user_client *user_client)
{
	// Start up is dumb. Windows requirement
	cnt_start_up();

	cnt_ip host_address;
	cnt_ip_create(&host_address, user_client->host_ip, user_client->host_port);

	cnt_sock client_socket;
	cnt_sock_open(&client_socket, CNT_ANY_IP, CNT_ANY_PORT);

	cnt_connection connection;
	cnt_connection_open(&connection, &client_socket, &host_address);

	cnt_client client;
	cnt_client_open(&client, &connection);

	cnt_transport transport;
	cnt_transport_open(&transport, 1 << 16);

	cnt_compression compression;
	cnt_compression_open(&compression, 1 << 16);

	cnt_stream stream;
	cnt_stream_open(&stream, 1024);

	// Tick
	while (true)
	{
		cnt_stream_clear(&transport.stream);
		if (cnt_client_send(&client, &transport.stream))
		{
			int transport_end = transport.stream.at;
			
			cnt_user_client_frame *frame;
			while (cnt_user_client_frame_concurrent_queue_try_dequeue(&user_client->send_queue, &frame))
			{
				transport.stream.at = transport_end;
				cnt_stream_write_string(&transport.stream, frame->data, frame->count);
				cnt_compress(&compression, &transport.stream);
				cnt_connection_send(&client.connection, &compression.stream);
				// User should do this too... For now
				cnt_user_client_frame_free(frame);
			}
		}
		else
		{
			cnt_compress(&compression, &transport.stream);
			cnt_connection_send(&client.connection, &compression.stream);
		}

		while (cnt_connection_recv(&client.connection, &compression.stream))
		{
			cnt_decompress(&compression, &transport.stream);

			cnt_stream recv_stream;
			cnt_stream_create(&recv_stream, transport.stream.data, transport.stream.at);

			if (cnt_client_recv(&client, &recv_stream))
			{
				cnt_stream frame_stream;
				cnt_stream_create_full(&frame_stream, recv_stream.data + recv_stream.at, recv_stream.capacity - recv_stream.at);

				cnt_user_client_frame *frame = cnt_user_client_frame_alloc(&frame_stream);
				cnt_user_client_frame_concurrent_queue_enqueue(&user_client->recv_queue, frame);
				cnt_user_client_frame_concurrent_queue_submit(&user_client->recv_queue);
			}
		}
		// Hardcoded for now, baby
		SDL_Delay(8);
	}

	// Cleanup
	cnt_stream_close(&stream);

	cnt_client_close(&client);

	cnt_compression_close(&compression);

	cnt_transport_close(&transport);

	cnt_connection_close(&connection);

	cnt_sock_close(&client_socket);

	cnt_tead_down();
	return 0;
}

int cnt_host_default_process(cnt_user_host *user_host)
{
	cnt_start_up();

	// Startup
	cnt_sock server_socket;
	cnt_sock_open(&server_socket, user_host->host_ip, user_host->host_port);

	cnt_star star;
	cnt_star_open(&star, &server_socket, 32);

	cnt_host host;
	cnt_host_open(&host, 32);

	cnt_message_64_bytes_queue message_queue;
	cnt_message_queue_64_bytes_open(&message_queue, 128);

	cnt_transport transport;
	cnt_transport_open(&transport, 1 << 16);

	cnt_compression compression;
	cnt_compression_open(&compression, 1 << 16);

	// Tick
	while (true)
	{
		cnt_user_host_command user_command;
		while (cnt_user_host_command_concurrent_queue_try_dequeue(&user_host->command_queue, &user_command))
		{
			switch (user_command.type)
			{
			case CNT_USER_HOST_COMMAND_TYPE_QUIT:
				break;
			case CNT_USER_HOST_COMMAND_TYPE_RESTART:
				break;
			case CNT_USER_HOST_COMMAND_TYPE_KICK:
				cnt_sparse_index client = user_command.kick.client;
				cnt_host_kick(&host, client);
				break;
			}
		}

		// Clear the message queue since we receive a new queue from host layer
		cnt_message_queue_64_bytes_clear(&message_queue);

		// Adds header to packet
		cnt_stream_clear(&transport.stream);
		cnt_host_send(&host, &transport.stream, &star.destinations, &message_queue);

		int transport_end = transport.stream.at;

		cnt_user_host_frame *frame;
		while (cnt_user_host_frame_concurrent_queue_try_dequeue(&user_host->send_queue, &frame))
		{
			transport.stream.at = transport_end;
			// Add data to transport
			// Send out packet
			if (frame->client_id.index != CNT_SPARSE_INDEX_INVALID)
			{
				if (cnt_host_is_client_connected(&host, frame->client_id))
				{
					cnt_stream_write_string(&transport.stream, frame->data, frame->count);
					cnt_compress(&compression, &transport.stream);

					cnt_ip ip;
					bool has_ip = cnt_host_client_ip_try_get(&host, frame->client_id, &ip);
					SDL_assert(has_ip && "IP does not exist, but state is valid");

					cnt_connection client_connection;
					cnt_connection_open(&client_connection, &star.sock, &ip);
					cnt_connection_send(&client_connection, &compression.stream);
				}
			}
			else
			{
				cnt_stream_write_string(&transport.stream, frame->data, frame->count);
				cnt_compress(&compression, &transport.stream);
				cnt_star_send(&star, &compression.stream);
			}
			// User should do this too... For now
			cnt_user_host_frame_free(frame);
		}

		cnt_connection message_connection;
		cnt_connection_from_socket(&message_connection, &star.sock);
		for (uint32_t index = 0; index < message_queue.count; index++)
		{
			cnt_message_64_bytes *message = &message_queue.messages[index];
			cnt_connection_set_destination(&message_connection, &message->ip);

			cnt_stream message_stream;
			cnt_stream_create_full(&message_stream, message->payload, message->payload_count);

			cnt_compress(&compression, &message_stream);
			cnt_connection_send(&message_connection, &compression.stream);
		}

		cnt_stream_clear(&transport.stream);

		cnt_ip recv_addr;
		while (cnt_star_recv(&star, &recv_addr, &compression.stream))
		{
			cnt_decompress(&compression, &transport.stream);

			cnt_stream recv_stream;
			cnt_stream_create(&recv_stream, transport.stream.data, transport.stream.at);

			cnt_client_on_host *client = cnt_host_recv(&host, &recv_addr, &recv_stream);
			if (client != nullptr)
			{
				cnt_stream frame_stream;
				cnt_stream_create_full(&frame_stream, recv_stream.data + recv_stream.at, recv_stream.capacity - recv_stream.at);
				// We probably need to differ between client frame and host frame...
				// Client KNOWS where it receives the data from
				// Host needs to propagate!
				cnt_user_host_frame *frame = cnt_user_host_frame_alloc(&frame_stream, client->id);
				cnt_user_host_frame_concurrent_queue_enqueue(&user_host->recv_queue, frame);
				cnt_user_host_frame_concurrent_queue_submit(&user_host->recv_queue);
			}
		}

		// Hardcoded for now, baby
		SDL_Delay(8);
	}

	// Cleanup
	cnt_compression_close(&compression);

	cnt_transport_close(&transport);

	cnt_message_queue_64_bytes_close(&message_queue);

	cnt_host_close(&host);

	cnt_star_close(&star);

	cnt_sock_close(&server_socket);

	cnt_tead_down();
	return 0;
}

bool TEST_cnt_user_host_frame_concurrent_queue()
{
	cnt_user_host_frame_concurrent_queue queue;
	cnt_user_host_frame_concurrent_queue_open(&queue, 16);

	int count = 4;
	for (int i = 0; i < count; i++)
	{
		cnt_user_host_frame_concurrent_queue_enqueue(&queue, nullptr);
	}

	cnt_user_host_frame_concurrent_queue_submit(&queue);

	cnt_user_host_frame *frame;
	while (cnt_user_host_frame_concurrent_queue_try_dequeue(&queue, &frame))
	{
		count = count - 1;
	}

	cnt_user_host_frame_concurrent_queue_close(&queue);

	return count == 0;
}