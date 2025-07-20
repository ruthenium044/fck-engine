#include "cnt_net.h"

#include "lz4.h"
#include <WinSock2.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#define cnt_assert(expr) assert(expr)
#define cnt_zero(ptr) memset(&ptr, 0, sizeof((ptr)))
#define cnt_zerop(ptr) memset(ptr, 0, sizeof(*(ptr)))
#define cnt_memcpy(dst, src, size) memcpy(dst, src, size)
#define cnt_log(str, ...) printf(str, __VA_ARGS__)

// TODO: Make network engine restartable!!
// TODO: Nice and dandy secret implementation
// TODO: Better and prettier logging
// TODO: Remove magic values, i.e. version constant - maybe just put them on top as constants
// Internal declaration for the default client/host processes
int cnt_client_default_process(cnt_user_client *);
int cnt_host_default_process(cnt_user_host *);

// Windows
#ifdef _WIN32
#include <ws2tcpip.h>
typedef struct cnt_sock_internal
{
	SOCKET handle;
} cnt_sock_internal;

typedef struct cnt_ip_internal
{
	union {
		ADDRESS_FAMILY family;   // Address family.
		struct sockaddr addr;    // base
		struct sockaddr_in in;   // ipv4
		struct sockaddr_in6 in6; // ipv6
	};
	fckc_u8 addrlen; // Fucking asshole Win32 doesn't have length
} cnt_ip_internal;
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
typedef struct cnt_sock_internal
{
	int handle;
} cnt_sock_internal;

typedef struct cnt_ip_internal // Not tested on Linux, only MacOS
{
	union {
		struct
		{
			fckc_u8 addrlen;    // Addrlen is part of BSD header info
			sa_family_t family; // Address family.
		};
		struct sockaddr addr;    // base
		struct sockaddr_in in;   // ipv4
		struct sockaddr_in6 in6; // ipv6
	};
} cnt_ip_internal;
#endif

// TODO: Replace these static asserts with C stuff
// static_assert(sizeof(cnt_ip) >= sizeof(cnt_ip_internal), "User-facing IP address is not large enough to hold internal IP address");
// static_assert(sizeof(cnt_sock) >= sizeof(cnt_sock_internal), "User-facing socket is not large enough to hold internal socket");

#if _WIN32
#include <winsock2.h>
static SDL_AtomicInt wsa_reference_counter = {0};
#endif // _WIN32

// TODO: Use it consistently
#define CNT_NULL_CHECK(var) cnt_assert((var) && #var " is null pointer")

#define CNT_FOR(type, var_name, count) for (type var_name = 0; (var_name) < (count); (var_name)++)
#define CNT_SIZEOF_SMALLER(a, b) sizeof(a) < sizeof(b) ? sizeof(a) : sizeof(b)

const char *cnt_protocol_info_name[] = {"CNT_PROTOCOL_STATE_COMMON_NONE",    "CNT_PROTOCOL_STATE_CLIENT_REQUEST",
                                        "CNT_PROTOCOL_STATE_HOST_CHALLENGE", "CNT_PROTOCOL_STATE_CLIENT_ANSWER",
                                        "CNT_PROTOCOL_STATE_COMMON_OK",      "CNT_PROTOCOL_STATE_HOST_RESOLUTION_REJECT",
                                        "CNT_PROTOCOL_STATE_HOST_KICKED"};

// This only works under the assumption that the user will never receive
// a tombstone sparse index to begin with. Let's pray
#define CNT_SPARSE_INDEX_INVALID ((fckc_u32)0xFFFFFFFF)
// static_assert(CNT_SPARSE_INDEX_INVALID == CNT_SPARSE_INDEX_INVALID);

void cnt_sparse_list_open(cnt_sparse_list *mapping, fckc_u32 capacity)
{
	const fckc_u32 maximum_possible_capacity = (CNT_SPARSE_INDEX_INVALID >> 1);

	CNT_NULL_CHECK(mapping);
	cnt_assert(capacity <= maximum_possible_capacity && "Capacity is too large. Stay below 0x7FFFFFFF");

	cnt_zerop(mapping);

	mapping->capacity = capacity;
	mapping->free_head = CNT_SPARSE_INDEX_INVALID;
	mapping->control_bit_mask = (CNT_SPARSE_INDEX_INVALID >> 1) + 1;

	mapping->sparse = (cnt_sparse_index *)SDL_malloc(sizeof(*mapping->sparse) * mapping->capacity);
	mapping->dense = (cnt_dense_index *)SDL_malloc(sizeof(*mapping->dense) * mapping->capacity);
	CNT_FOR(fckc_u32, index, mapping->capacity)
	{
		mapping->sparse[index].index = CNT_SPARSE_INDEX_INVALID;
	}
}

void cnt_sparse_list_close(cnt_sparse_list *mapping)
{
	CNT_NULL_CHECK(mapping);

	SDL_free(mapping->sparse);
	SDL_free(mapping->dense);

	cnt_zerop(mapping);
}

void cnt_sparse_list_make_invalid(cnt_sparse_list *mapping, cnt_sparse_index *index)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(index);

	const fckc_u32 control_bit_mask = mapping->control_bit_mask;
	index->index = index->index | control_bit_mask;
}

void cnt_sparse_list_make_valid(cnt_sparse_list *mapping, cnt_sparse_index *index)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(index);

	const fckc_u32 control_bit_mask = mapping->control_bit_mask;
	index->index = (index->index & ~control_bit_mask);
}

int cnt_sparse_list_exists(cnt_sparse_list *mapping, cnt_sparse_index *index)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(index);

	if (index->index >= mapping->capacity)
	{
		return 0;
	}

	cnt_sparse_index *slot = &mapping->sparse[index->index];
	const fckc_u32 control_bit_mask = mapping->control_bit_mask;
	if ((slot->index & control_bit_mask) == control_bit_mask)
	{
		return 0;
	}

	return 1;
}

int cnt_sparse_list_try_get_sparse(cnt_sparse_list *mapping, cnt_dense_index *index, cnt_sparse_index *out)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(out);

	if (index->index >= mapping->count)
	{
		return 0;
	}

	cnt_dense_index *dense = &mapping->dense[index->index];
	out->index = dense->index;

	return 1;
}

int cnt_sparse_list_try_get_dense(cnt_sparse_list *mapping, cnt_sparse_index *index, cnt_dense_index *out)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(out);

	if (!cnt_sparse_list_exists(mapping, index))
	{
		out = NULL;
		return 0;
	}

	cnt_sparse_index *sparse = &mapping->sparse[index->index];
	out->index = sparse->index;

	return 1;
}

int cnt_sparse_list_create(cnt_sparse_list *mapping, cnt_sparse_index *out)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(out);

	cnt_zerop(out);

	if (mapping->count >= mapping->capacity)
	{
		return 0;
	}

	fckc_u32 next_dense = mapping->count;

	int has_free_slot = mapping->free_head != CNT_SPARSE_INDEX_INVALID;
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
	return 1;
}

int cnt_sparse_list_remove(cnt_sparse_list *mapping, cnt_sparse_index *index)
{
	CNT_NULL_CHECK(mapping);

	if (!cnt_sparse_list_exists(mapping, index))
	{
		return 0;
	}

	// Update mapping
	{

		cnt_sparse_index *current_sparse = &mapping->sparse[index->index];
		cnt_dense_index *current_dense = &mapping->dense[current_sparse->index];
		cnt_assert(index->index == current_dense->index && "Mix-up happened with the mapping?!");

		fckc_u32 last_dense = mapping->count - 1;
		cnt_dense_index *last_dense_index = &mapping->dense[last_dense];
		cnt_sparse_index *last_sparse_index = &mapping->sparse[last_dense_index->index];
		cnt_assert(last_dense == last_sparse_index->index && "Mix-up happened with the mapping?!");

		current_dense->index = last_dense_index->index;
		last_sparse_index->index = current_sparse->index;
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
	return 1;
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
	cnt_assert(reference_counter >= 0);

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
	cnt_zerop(dst);
	cnt_memcpy(dst, sock, CNT_SIZEOF_SMALLER(*dst, *sock));

	return dst;
}

static cnt_sock_internal *copy_to_internal_sock(cnt_sock *sock, cnt_sock_internal *dst)
{
	cnt_zerop(dst);
	cnt_memcpy(dst, sock, CNT_SIZEOF_SMALLER(*dst, *sock));

	return dst;
}

static cnt_ip *copy_to_user_address(cnt_ip_internal *addr, cnt_ip *dst)
{
	cnt_zerop(dst);
	cnt_memcpy(dst, addr, CNT_SIZEOF_SMALLER(*dst, *addr));

	return dst;
}

static cnt_ip_internal *copy_to_internal_address(cnt_ip *addr, cnt_ip_internal *dst)
{
	cnt_zerop(dst);
	cnt_memcpy(dst, addr, CNT_SIZEOF_SMALLER(*dst, *addr));

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

static int socket_is_valid(cnt_sock_internal *socket)
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

static int would_block(const int err)
{
#ifdef _WIN32
	return (err == WSAEWOULDBLOCK) ? 1 : 0;
#else
	return ((err == EWOULDBLOCK) || (err == EAGAIN) || (err == EINPROGRESS)) ? 1 : 0;
#endif
}

static int cnt_ip_is_empty(cnt_ip *ip)
{
	CNT_NULL_CHECK(ip);

	cnt_ip_internal ip_internal;
	copy_to_internal_address(ip, &ip_internal);

	return ip_internal.addrlen == 0;
}

static int cnt_ip_equals(cnt_ip *a, cnt_ip *b)
{
	if (a == NULL || b == NULL)
	{
		return 0;
	}

	cnt_ip_internal lhs;
	copy_to_internal_address(a, &lhs);
	cnt_ip_internal rhs;
	copy_to_internal_address(b, &rhs);

	if (lhs.addrlen != rhs.addrlen)
	{
		return 0;
	}

	return SDL_memcmp(&lhs.addr, &rhs.addr, lhs.addrlen) == 0;
}

cnt_ip *cnt_ip_create(cnt_ip *address, const char *ip, fckc_u16 port)
{
	cnt_assert(address && "Address is null pointer");

	// We only accept
	fckc_u8 buffer[128];
	int result = inet_pton(AF_INET, ip, &buffer);
	CHECK_ERROR(result == 1, "Cannot convert address from text to binary - invalid arguments", return {});

	cnt_ip_internal result_address;
	cnt_zero(result_address);

	result_address.in.sin_family = AF_INET;
	result_address.in.sin_addr = *(struct in_addr *)buffer; // Prefer memcpy
	result_address.in.sin_port = port;
	result_address.addrlen = sizeof(struct sockaddr_in);

	copy_to_user_address(&result_address, address);

	return address;
}

static cnt_ip_container *cnt_ip_container_open(cnt_ip_container *container, fckc_u32 capacity)
{
	cnt_assert(container && "Container is null pointer");

	container->addresses = (cnt_ip *)SDL_malloc(sizeof(*container->addresses) * capacity);
	container->capacity = capacity;
	container->count = 0;
	return container;
}

static cnt_ip *cnt_ip_container_add(cnt_ip_container *container, cnt_ip *address)
{
	cnt_assert(container && "Container is null pointer");

	// Add new address if not found!
	fckc_u32 at = container->count;
	cnt_memcpy(container->addresses + at, address, sizeof(*address));
	container->count = at + 1;
	return container->addresses + at;
}

int cnt_ip_container_index_of(cnt_ip_container *container, cnt_ip *address, fckc_u32 *index)
{
	cnt_assert(container && "Container is null pointer");

	CNT_FOR(fckc_u32, i, container->count)
	{
		if (cnt_ip_equals(container->addresses + i, address))
		{
			*index = i;
			return 1;
		}
	}

	return 0;
}

static void cnt_ip_container_remove_swap_at(cnt_ip_container *container, fckc_u32 index)
{
	cnt_assert(container && "Container is null pointer");

	fckc_u32 last = container->count - 1;

	cnt_memcpy(container->addresses + index, container->addresses + last, sizeof(*container->addresses));
	container->count = last;
}

static void cnt_ip_container_clear(cnt_ip_container *container)
{
	cnt_assert(container && "Container is null pointer");

	container->count = 0;
}

static void cnt_ip_container_close(cnt_ip_container *container)
{
	cnt_assert(container && "Container is null pointer");

	SDL_free(container->addresses);
	cnt_zerop(container);
}

cnt_sock *cnt_sock_open(cnt_sock *sock, const char *ip, fckc_u16 port)
{
	cnt_assert(sock && "Sock is null pointer");

	cnt_ip address;
	if (cnt_ip_create(&address, ip, port) == NULL)
	{
		return NULL;
	}

	cnt_ip_internal addr_internal;
	copy_to_internal_address(&address, &addr_internal);

	cnt_sock_internal sock_internal;
	sock_internal.handle = socket(addr_internal.family, SOCK_DGRAM, 0);
	CHECK_CRITICAL(socket_is_valid(&sock_internal), "Failed to create socket", return NULL);

	const int non_block_sucess = make_socket_non_blocking(&sock_internal) >= 0;
	CHECK_CRITICAL(non_block_sucess, "Failed to make socket non-blocking", close_socket(&sock_internal); return NULL);

	const int binding_failed = bind(sock_internal.handle, &addr_internal.addr, addr_internal.addrlen);
	CHECK_CRITICAL(binding_failed != -1, "Failed to bint socket", close_socket(&sock_internal); return NULL);

	copy_to_user_sock(&sock_internal, sock);

	return sock;
}

void cnt_sock_close(cnt_sock *sock)
{
	cnt_assert(sock && "Sock is null pointer");

	cnt_sock_internal sock_internal;
	copy_to_internal_sock(sock, &sock_internal);
	close_socket(&sock_internal);

	cnt_zerop(sock);
}

cnt_message_64_bytes_queue *cnt_message_queue_64_bytes_open(cnt_message_64_bytes_queue *queue, fckc_u32 capacity)
{
	CNT_NULL_CHECK(queue);

	cnt_zerop(queue);

	queue->capacity = capacity;
	queue->messages = (cnt_message_64_bytes *)SDL_malloc(sizeof(*queue->messages) * capacity);

	return queue;
}

cnt_message_64_bytes *cnt_message_queue_64_bytes_push(cnt_message_64_bytes_queue *queue, cnt_message_64_bytes const *message)
{
	CNT_NULL_CHECK(queue);
	if (queue->count >= queue->capacity)
	{
		return NULL;
	}

	cnt_memcpy(queue->messages + queue->count, message, sizeof(*queue->messages));

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

	cnt_zerop(queue);
}

cnt_stream *cnt_stream_create(cnt_stream *stream, fckc_u8 *data, int capacity)
{
	CNT_NULL_CHECK(stream);

	stream->data = data;
	stream->capacity = capacity;
	stream->at = 0;
	return stream;
}

// Find better name - Buffer with at set to capacity
cnt_stream *cnt_stream_create_full(cnt_stream *stream, fckc_u8 *data, int capacity)
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

	stream->data = (fckc_u8 *)SDL_malloc(capacity);
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
	cnt_assert(stream && "Stream is null pointer");

	SDL_free(stream->data);
	cnt_zerop(stream);
}

int cnt_stream_can_read(cnt_stream *stream, size_t size)
{
	return SDL_max(stream->capacity - stream->at, 0) >= size;
}

static void cnt_stream_append(cnt_stream *serialiser, fckc_u8 *value, fckc_u16 count)
{
	fckc_u8 *at = serialiser->data + serialiser->at;

	cnt_memcpy(at, value, count);

	serialiser->at = serialiser->at + (sizeof(*value) * count);
}

static void cnt_stream_string_length(cnt_stream *serialiser, size_t *count)
{
	fckc_u8 const *at = serialiser->data + serialiser->at;

	size_t max_len = SDL_max(serialiser->capacity - serialiser->at, 0);
	*count = SDL_strnlen((const char *)at, max_len);
}

static void cnt_stream_peek_string(cnt_stream *serialiser, fckc_u8 *value, fckc_u16 count)
{
	cnt_assert(serialiser->at + count <= serialiser->capacity);

	fckc_u8 *at = serialiser->data + serialiser->at;

	cnt_memcpy(value, at, count);
}
static void cnt_stream_peek_uint8(cnt_stream *serialiser, fckc_u8 *value)
{
	cnt_assert(serialiser->at + sizeof(*value) <= serialiser->capacity);

	fckc_u8 *at = serialiser->data + serialiser->at;

	*value = *at;
}
static void cnt_stream_peek_uint16(cnt_stream *serialiser, fckc_u16 *value)
{
	cnt_assert(serialiser->at + sizeof(*value) <= serialiser->capacity);

	fckc_u8 *at = serialiser->data + serialiser->at;

	*value = ((fckc_u16)at[0] << 0)    // 0xFF000000
	         | ((fckc_u16)at[1] << 8); // 0x00FF0000
}
static void cnt_stream_peek_uint32(cnt_stream *serialiser, fckc_u32 *value)
{
	cnt_assert(serialiser->at + sizeof(*value) <= serialiser->capacity);

	fckc_u8 *at = serialiser->data + serialiser->at;

	*value = ((fckc_u32)(at[0]) << 0)     // 0xFF000000
	         | ((fckc_u32)(at[1]) << 8)   // 0x00FF0000
	         | ((fckc_u32)(at[2]) << 16)  // 0x0000FF00
	         | ((fckc_u32)(at[3]) << 24); // 0x000000FF
}
static void cnt_stream_peek_uint64(cnt_stream *serialiser, fckc_u64 *value)
{
	cnt_assert(serialiser->at + sizeof(*value) <= serialiser->capacity);

	fckc_u8 *at = serialiser->data + serialiser->at;

	*value = ((fckc_u64)at[0] << 0ull)     // 0xFF00000000000000
	         | ((fckc_u64)at[1] << 8ull)   // 0x00FF000000000000
	         | ((fckc_u64)at[2] << 16ull)  // 0x0000FF0000000000
	         | ((fckc_u64)at[3] << 24ull)  // 0x000000FF00000000
	         | ((fckc_u64)at[4] << 32ull)  // 0x00000000FF000000
	         | ((fckc_u64)at[5] << 40ull)  // 0x0000000000FF0000
	         | ((fckc_u64)at[6] << 48ull)  // 0x000000000000FF00
	         | ((fckc_u64)at[7] << 56ull); // 0x00000000000000FF
}

static void cnt_stream_read_string(cnt_stream *serialiser, void *value, fckc_u16 count)
{
	cnt_assert(serialiser->at + count <= serialiser->capacity);

	fckc_u8 *at = serialiser->data + serialiser->at;

	cnt_memcpy(value, at, count);

	serialiser->at = serialiser->at + count;
}
static void cnt_stream_read_uint8(cnt_stream *serialiser, fckc_u8 *value)
{
	cnt_assert(serialiser->at + sizeof(*value) <= serialiser->capacity);

	fckc_u8 *at = serialiser->data + serialiser->at;

	*value = *at;
	serialiser->at = serialiser->at + sizeof(*value);
}
static void cnt_stream_read_uint16(cnt_stream *serialiser, fckc_u16 *value)
{
	cnt_assert(serialiser->at + sizeof(*value) <= serialiser->capacity);

	fckc_u8 *at = serialiser->data + serialiser->at;

	*value = ((fckc_u16)at[0] << 0)    // 0xFF000000
	         | ((fckc_u16)at[1] << 8); // 0x00FF0000

	serialiser->at = serialiser->at + sizeof(*value);
}
static void cnt_stream_read_uint32(cnt_stream *serialiser, fckc_u32 *value)
{
	cnt_assert(serialiser->at + sizeof(*value) <= serialiser->capacity);

	fckc_u8 *at = serialiser->data + serialiser->at;

	*value = ((fckc_u32)(at[0]) << 0)     // 0xFF000000
	         | ((fckc_u32)(at[1]) << 8)   // 0x00FF0000
	         | ((fckc_u32)(at[2]) << 16)  // 0x0000FF00
	         | ((fckc_u32)(at[3]) << 24); // 0x000000FF

	serialiser->at = serialiser->at + sizeof(*value);
}
static void cnt_stream_read_uint64(cnt_stream *serialiser, fckc_u64 *value)
{
	cnt_assert(serialiser->at + sizeof(*value) <= serialiser->capacity);

	fckc_u8 *at = serialiser->data + serialiser->at;

	*value = ((fckc_u64)at[0] << 0ull)     // 0xFF00000000000000
	         | ((fckc_u64)at[1] << 8ull)   // 0x00FF000000000000
	         | ((fckc_u64)at[2] << 16ull)  // 0x0000FF0000000000
	         | ((fckc_u64)at[3] << 24ull)  // 0x000000FF00000000
	         | ((fckc_u64)at[4] << 32ull)  // 0x00000000FF000000
	         | ((fckc_u64)at[5] << 40ull)  // 0x0000000000FF0000
	         | ((fckc_u64)at[6] << 48ull)  // 0x000000000000FF00
	         | ((fckc_u64)at[7] << 56ull); // 0x00000000000000FF

	serialiser->at = serialiser->at + sizeof(*value);
}

static void cnt_stream_write_string(cnt_stream *serialiser, void const *value, fckc_u16 count)
{
	fckc_u8 *at = serialiser->data + serialiser->at;

	cnt_memcpy(at, value, count);

	serialiser->at = serialiser->at + count;
}
static void cnt_stream_write_uint8(cnt_stream *serialiser, fckc_u8 value)
{
	fckc_u8 *at = serialiser->data + serialiser->at;

	*at = value;

	serialiser->at = serialiser->at + sizeof(value);
}
static void cnt_stream_write_uint16(cnt_stream *serialiser, fckc_u16 value)
{
	fckc_u8 *at = serialiser->data + serialiser->at;

	at[0] = (fckc_u8)((value >> 0) & 0xFF);
	at[1] = (fckc_u8)((value >> 8) & 0xFF);

	serialiser->at = serialiser->at + sizeof(value);
}
static void cnt_stream_write_uint32(cnt_stream *serialiser, fckc_u32 value)
{
	fckc_u8 *at = serialiser->data + serialiser->at;

	at[0] = (fckc_u8)((value >> 0) & 0xFF);
	at[1] = (fckc_u8)((value >> 8) & 0xFF);
	at[2] = (fckc_u8)((value >> 16) & 0xFF);
	at[3] = (fckc_u8)((value >> 24) & 0xFF);

	serialiser->at = serialiser->at + sizeof(value);
}
static void cnt_stream_write_uint64(cnt_stream *serialiser, fckc_u64 value)
{
	fckc_u8 *at = serialiser->data + serialiser->at;

	at[0] = (fckc_u8)((value >> 0) & 0xFF);
	at[1] = (fckc_u8)((value >> 8) & 0xFF);
	at[2] = (fckc_u8)((value >> 16) & 0xFF);
	at[3] = (fckc_u8)((value >> 24) & 0xFF);
	at[4] = (fckc_u8)((value >> 32) & 0xFF);
	at[5] = (fckc_u8)((value >> 40) & 0xFF);
	at[6] = (fckc_u8)((value >> 48) & 0xFF);
	at[7] = (fckc_u8)((value >> 56) & 0xFF);

	serialiser->at = serialiser->at + sizeof(value);
}

cnt_transport *cnt_transport_open(cnt_transport *transport, int capacity)
{
	cnt_assert(transport && "Transport is null pointer");

	cnt_stream_open(&transport->stream, capacity);
	return transport;
}

void cnt_transport_close(cnt_transport *transport)
{
	cnt_assert(transport && "Transport is null pointer");

	cnt_stream_close(&transport->stream);
	cnt_zerop(transport);
}

cnt_compression *cnt_compression_open(cnt_compression *compression, fckc_u32 capacity)
{
	cnt_assert(compression && "Compression is null pointer");

	cnt_stream_open(&compression->stream, capacity);
	return compression;
}

cnt_compression *cnt_compress(cnt_compression *compression, cnt_stream *stream)
{
	cnt_assert(compression && "Compression is null pointer");
	cnt_assert(stream && "Stream is null pointer");

	int result = LZ4_compress_default((char *)stream->data, (char *)compression->stream.data, stream->at, compression->stream.capacity);
	if (result >= 0)
	{
		compression->stream.at = result;
	}
	return compression;
}

cnt_stream *cnt_decompress(cnt_compression *compression, cnt_stream *stream)
{
	cnt_assert(compression && "Compression is null pointer");
	cnt_assert(stream && "Stream is null pointer");

	int result = LZ4_decompress_safe((char *)compression->stream.data, (char *)stream->data, compression->stream.at, stream->capacity);
	if (result >= 0)
	{
		stream->at = result;
	}
	return stream;
}

void cnt_compression_close(cnt_compression *compression)
{
	cnt_assert(compression && "Compression is null pointer");

	cnt_stream_close(&compression->stream);
	cnt_zerop(compression);
}

static void cnt_protocol_client_write(cnt_protocol_client *packet, cnt_stream *stream)
{
	cnt_stream_write_uint32(stream, packet->prefix);
	cnt_stream_write_uint8(stream, (fckc_u8)packet->state);
	cnt_stream_write_uint32(stream, packet->id.index);

	cnt_stream_write_uint8(stream, packet->extra_payload_count);
	cnt_stream_write_string(stream, packet->extra_payload, packet->extra_payload_count);
}

static int cnt_protocol_client_read(cnt_protocol_client *packet, cnt_stream *stream)
{
	if (!cnt_stream_can_read(stream, sizeof(packet->prefix)))
	{
		return 0;
	}

	cnt_stream_read_uint32(stream, &packet->prefix);
	if (packet->prefix != 8008)
	{
		// We can just stop here. The packet is illegal af
		return 0;
	}
	cnt_stream_read_uint8(stream, (fckc_u8 *)&packet->state);
	cnt_stream_read_uint32(stream, &packet->id.index);
	cnt_stream_read_uint8(stream, &packet->extra_payload_count);
	cnt_stream_read_string(stream, packet->extra_payload, packet->extra_payload_count);
	return 1;
}

static void cnt_protocol_host_write(cnt_protocol_host *packet, cnt_stream *stream)
{
	cnt_stream_write_uint32(stream, packet->prefix);
	cnt_stream_write_uint8(stream, (fckc_u8)packet->state);
	cnt_stream_write_uint8(stream, packet->extra_payload_count);
	cnt_stream_write_string(stream, packet->extra_payload, packet->extra_payload_count);
}

static int cnt_protocol_host_read(cnt_protocol_host *packet, cnt_stream *stream)
{
	if (!cnt_stream_can_read(stream, sizeof(packet->prefix)))
	{
		return 0;
	}

	cnt_stream_read_uint32(stream, &packet->prefix);
	if (packet->prefix != 8008)
	{
		// We can just stop here. The packet is illegal af
		return 0;
	}
	cnt_stream_read_uint8(stream, (fckc_u8 *)&packet->state);
	cnt_stream_read_uint8(stream, &packet->extra_payload_count);
	cnt_stream_read_string(stream, packet->extra_payload, packet->extra_payload_count);

	return 1;
}

static void cnt_protocol_secret_write(cnt_secret *secret, cnt_stream *stream)
{
	cnt_stream_write_uint8(stream, (fckc_u8)secret->state);

	cnt_stream_write_uint16(stream, secret->public_value);
}

static void cnt_protocol_secret_read(cnt_secret *secret, cnt_stream *stream)
{
	cnt_stream_read_uint8(stream, (fckc_u8 *)&secret->state);

	cnt_stream_read_uint16(stream, &secret->public_value);
}

cnt_connection *cnt_connection_from_socket(cnt_connection *connection, cnt_sock *src)
{
	cnt_assert(connection && "Connection is null");
	cnt_assert(src && "Socket (src) is null pointer");

	cnt_memcpy(&connection->src, src, sizeof(*src));
	cnt_zero(connection->dst);

	return connection;
}

cnt_connection *cnt_connection_set_destination(cnt_connection *connection, cnt_ip *dst)
{
	cnt_assert(connection && "Connection is null");
	cnt_assert(dst && "Socket (src) is null pointer");

	cnt_memcpy(&connection->dst, dst, sizeof(*dst));

	return connection;
}

cnt_connection *cnt_connection_open(cnt_connection *connection, cnt_sock *src, cnt_ip *dst)
{
	cnt_assert(connection && "Connection is null");
	cnt_assert(src && "Socket (src) is null pointer");
	cnt_assert(dst && "Address (dst) is null pointer");

	cnt_memcpy(&connection->src, src, sizeof(*src));
	cnt_memcpy(&connection->dst, dst, sizeof(*dst));
	return connection;
}

cnt_connection *cnt_connection_send(cnt_connection *connection, cnt_stream *stream)
{
	cnt_assert(connection && "Connection is null pointer");
	cnt_assert(stream && "Stream is null pointer");

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
		address.addrlen = sizeof(struct sockaddr_in);
		break;
	case AF_INET6:
		address.addrlen = sizeof(struct sockaddr_in6);
		break;
	default:
		cnt_zerop(out_address);
		break;
	}

	if (address.family == AF_INET6)
	{
		// We do not accept ipv6 for now
		cnt_zerop(out_address);
		return;
	}

	copy_to_user_address(&address, out_address);
}

cnt_connection *cnt_connection_recv(cnt_connection *connection, cnt_stream *stream)
{
	cnt_assert(connection && "Connection is null pointer");
	cnt_assert(stream && "Stream is null pointer");

	cnt_sock_internal sock_internal;
	copy_to_internal_sock(&connection->src, &sock_internal);

	cnt_sockaddr_storage storage;
	cnt_socklen socket_length = sizeof(storage);

	int result = recvfrom(sock_internal.handle, (char *)stream->data, stream->capacity, 0, (struct sockaddr *)&storage, &socket_length);

	if (result == -1)
	{
		const int err = last_socket_error();
		if (would_block(err))
		{
			stream->at = 0;
		}
		return NULL;
	}

	stream->at = result;

	query_address(&storage, &connection->dst);
	return connection;
}

void cnt_connection_close(cnt_connection *connection)
{
	cnt_assert(connection && "Connection is null pointer");

	cnt_zerop(connection);
}

static cnt_ip *cnt_star_find(cnt_star *star, cnt_ip *address)
{
	CNT_FOR(fckc_u32, index, star->destinations.count)
	{
		cnt_ip *destination = star->destinations.addresses + index;
		if (cnt_ip_equals(destination, address))
		{
			return destination;
		}
	}
	return NULL;
}

cnt_star *cnt_star_open(cnt_star *star, cnt_sock *sock, fckc_u32 max_connections)
{
	cnt_assert(star && "Star is null pointer");
	cnt_assert(sock && "Src is null pointer");

	cnt_memcpy(&star->sock, sock, sizeof(star->sock));

	cnt_ip_container_open(&star->destinations, max_connections);

	return star;
}

cnt_star *cnt_star_send(cnt_star *star, cnt_stream *stream)
{
	cnt_assert(star && "Star is null pointer");
	cnt_assert(stream && "Stream is null pointer");

	CNT_FOR(fckc_u32, index, star->destinations.count)
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
	cnt_assert(star && "Star is null pointer");
	cnt_assert(address && "Address is null pointer");
	cnt_assert(stream && "Stream is null pointer");

	cnt_connection connection;
	cnt_connection_from_socket(&connection, &star->sock);

	if (cnt_connection_recv(&connection, stream))
	{
		cnt_ip *addr = cnt_star_find(star, &connection.dst);
		if (addr == NULL)
		{
			// Add new address if not found!
			cnt_ip_container_add(&star->destinations, &connection.dst);
		}

		cnt_memcpy(address, &connection.dst, sizeof(connection.dst));
		return address;
	}
	return NULL;
}

cnt_star *cnt_star_add(cnt_star *star, cnt_ip *addr)
{
	cnt_assert(star && "Star is null pointer");
	cnt_assert(addr && "Addr is null pointer");

	cnt_assert(cnt_star_find(star, addr) != NULL && "Addr is already added");

	cnt_ip_container_add(&star->destinations, addr);

	return star;
}

cnt_ip *cnt_star_remove(cnt_star *star, cnt_ip *addr)
{
	cnt_assert(star && "Star is null pointer");
	cnt_assert(addr && "Addr is null pointer");

	CNT_FOR(fckc_u32, index, star->destinations.count)
	{
		cnt_ip *destination = star->destinations.addresses + index;
		if (cnt_ip_equals(destination, addr))
		{
			cnt_ip_container_remove_swap_at(&star->destinations, index);
			return addr;
		}
	}

	return NULL;
}

void cnt_star_close(cnt_star *star)
{
	cnt_assert(star && "Star is null pointer");

	cnt_ip_container_close(&star->destinations);

	cnt_sock_close(&star->sock);

	cnt_zerop(star);
}

int cnt_host_ip_try_find(cnt_host *host, cnt_ip *ip, fckc_u32 *at)
{
	// This is hackable
	for (fckc_u32 index = 0; index < (host->mapping.count); index++)
	{
		cnt_ip *address = &host->ip_lookup[index];
		if (cnt_ip_equals(address, ip))
		{
			*at = index;
			return 1;
		}
	}
	return 0;
}

cnt_protocol_client *cnt_protocol_client_create(cnt_protocol_client *client_protocol, cnt_client *client)
{
	cnt_assert(client && "Client is null pointer");
	cnt_assert(client_protocol && "Protocol is null pointer");
	// cnt_assert(client->protocol != CNT_PROTOCOL_STATE_CLIENT_NONE && "Client is not allowed to be in NONE state");

	cnt_zerop(client_protocol);

	client_protocol->prefix = 8008;
	client_protocol->state = client->protocol;
	client_protocol->id = client->id_on_host;

	client->attempts = client->attempts + 1;

	switch (client_protocol->state)
	{
		// In any case, we send our secret over
	case CNT_PROTOCOL_STATE_CLIENT_REQUEST:
	case CNT_PROTOCOL_STATE_CLIENT_ANSWER:
	case CNT_PROTOCOL_STATE_CLIENT_DISCONNECT:
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

const fckc_u16 SECRET_SEED = 69;

cnt_client_on_host *cnt_protocol_client_apply(cnt_protocol_client *client_protocol, cnt_host *host, cnt_ip *client_addr)
{
	cnt_assert(host && "Host is null pointer");
	cnt_assert(client_protocol && "Protocol is null pointer");
	cnt_assert(client_protocol->prefix == 8008);
	int should_drop = client_protocol->state == CNT_PROTOCOL_STATE_HOST_NONE || client_protocol->state == CNT_PROTOCOL_STATE_CLIENT_NONE;
	if (should_drop)
	{
		return NULL;
	}

	cnt_dense_index dense_index;
	int client_id_exists = cnt_sparse_list_try_get_dense(&host->mapping, &client_protocol->id, &dense_index);
	if (client_protocol->state == CNT_PROTOCOL_STATE_CLIENT_DISCONNECT)
	{
		if (client_id_exists)
		{
			cnt_ip *stored_addr = &host->ip_lookup[dense_index.index];
			int is_stored_addr_mapped_correctly = cnt_ip_equals(stored_addr, client_addr);
			if (is_stored_addr_mapped_correctly)
			{
				cnt_stream stream;
				cnt_stream_create(&stream, client_protocol->extra_payload, client_protocol->extra_payload_count);

				cnt_client_on_host *client_state = &host->client_states[dense_index.index];
				if (client_state->protocol == CNT_PROTOCOL_STATE_HOST_OK)
				{
					// If the client disconnects while established, check the secret
					cnt_protocol_secret_read(&client_state->secret, &stream);
					int is_secret_correct = (client_state->secret.public_value ^ client_state->secret.private_value) == SECRET_SEED;
					if (is_secret_correct)
					{
						client_state->protocol = CNT_PROTOCOL_STATE_HOST_DISCONNECT;
						return NULL;
					}
				}
				else
				{
					// If the client disconnects during the handshake, just invalidate!
					// Packets for the handshake can still be in-flight :(
					client_state->protocol = CNT_PROTOCOL_STATE_HOST_DISCONNECT;
					return NULL;
				}
			}
			cnt_log("Received naughty state from client on Host - Tried to disconnect another client?");
		}

		return NULL;
	}

	if (client_protocol->state == CNT_PROTOCOL_STATE_CLIENT_OK)
	{
		// Any kind of client can just send as an OK
		// When a client sends an OK, we just verify a few data entries
		if (!client_id_exists)
		{
			return NULL;
		}

		cnt_ip *stored_addr = &host->ip_lookup[dense_index.index];
		int is_stored_addr_mapped_correctly = cnt_ip_equals(stored_addr, client_addr);
		if (!is_stored_addr_mapped_correctly)
		{
			return NULL;
		}

		cnt_stream stream;
		cnt_stream_create(&stream, client_protocol->extra_payload, client_protocol->extra_payload_count);

		cnt_client_on_host *client_state = &host->client_states[dense_index.index];
		if (client_state->protocol == CNT_PROTOCOL_STATE_HOST_OK)
		{
			cnt_protocol_secret_read(&client_state->secret, &stream);
			int is_secret_correct = (client_state->secret.public_value ^ client_state->secret.private_value) == SECRET_SEED;
			if (is_secret_correct)
			{
				client_state->attempts = 0;
				return client_state;
			}
		}

		if (cnt_host_ip_try_find(host, client_addr, &dense_index.index))
		{
			// Client sent us bad data - we cannot change server state just because some client is a cunt
			cnt_log("Received naughty state from client on Host - Tried to circumvent handshake?");
		}
		return NULL;
	}

	// if (!client_id_exists)
	//{
	if (!cnt_host_ip_try_find(host, client_addr, &dense_index.index))
	{
		cnt_sparse_index id;
		if (!cnt_sparse_list_create(&host->mapping, &id))
		{
			cnt_log("Host is full - Cannot take in client. Bummer :(");
			return NULL;
		}
		int has_id = cnt_sparse_list_try_get_dense(&host->mapping, &id, &dense_index);
		cnt_assert(has_id);

		cnt_memcpy(host->ip_lookup + dense_index.index, client_addr, sizeof(*host->ip_lookup));

		cnt_client_on_host *client_state = &host->client_states[dense_index.index];
		cnt_zerop(client_state);
		client_state->id = id;
	}
	else
	{
		// Internal fuck ups
		cnt_sparse_index id;
		int has_sparse_index = cnt_sparse_list_try_get_sparse(&host->mapping, &dense_index, &id);
		cnt_assert(has_sparse_index && "Address exists in lookup, but did not get shoved into lookup");

		cnt_client_on_host *client_state = &host->client_states[dense_index.index];
		int is_stored_id_correct = client_state->id.index == id.index;
		cnt_assert(is_stored_id_correct && "Client-Address mapping is completely fucked up");
	}
	//}

	cnt_ip *stored_addr = &host->ip_lookup[dense_index.index];
	int is_stored_addr_empty = cnt_ip_is_empty(stored_addr);
	if (!is_stored_addr_empty)
	{
		int is_stored_addr_mapped_correctly = cnt_ip_equals(stored_addr, client_addr);
		if (!is_stored_addr_mapped_correctly)
		{
			// This can actually happen if the client (dumb) sends us 0.
			// Maybe we should reject with a reason, telling the client that its protocol initalisation failed
			cnt_log("Received naughty state from client on Host - Client impersonates?");
			return NULL;
		}
	}

	cnt_client_on_host *client_state = &host->client_states[dense_index.index];
	client_state->attempts = 0;

	if (client_state->protocol > client_protocol->state)
	{
		return NULL;
	}

	if (client_protocol->state == CNT_PROTOCOL_STATE_CLIENT_REQUEST)
	{
		client_state->secret.private_value = 420;
		client_state->secret.public_value = SECRET_SEED ^ client_state->secret.private_value;
		client_state->secret.state = CNT_SECRET_STATE_OUTDATED;
		client_state->protocol = CNT_PROTOCOL_STATE_HOST_CHALLENGE;
		cnt_log("Client (%lu) requests to join\nSend Challenge to Client (%lu)", client_state->id.index, client_state->id.index);

		return NULL;
	}

	if (client_protocol->state == CNT_PROTOCOL_STATE_CLIENT_ANSWER)
	{
		cnt_stream stream;
		cnt_stream_create(&stream, client_protocol->extra_payload, client_protocol->extra_payload_count);
		cnt_protocol_secret_read(&client_state->secret, &stream);
		fckc_u16 client_value = SECRET_SEED ^ client_state->secret.public_value;
		int is_correct = client_state->secret.private_value == client_value;

		client_state->secret.state = is_correct ? CNT_SECRET_STATE_ACCEPTED : CNT_SECRET_STATE_REJECTED;
		client_state->protocol = is_correct ? CNT_PROTOCOL_STATE_HOST_OK : CNT_PROTOCOL_STATE_HOST_RESOLUTION_REJECT;
		cnt_log("Client (%lu) answered to challenge\nSend response to Client (%lu) - %s", client_state->id.index, client_state->id.index,
		        is_correct ? "Accepted" : "Rejected");

		return NULL;
	}

	return NULL;
}

cnt_client *cnt_protocol_host_apply(cnt_protocol_host *host_protocol, cnt_client *client)
{
	cnt_assert(client && "Client is null pointer");
	cnt_assert(host_protocol && "Protocol is null pointer");
	cnt_assert(host_protocol->prefix == 8008);
	int should_drop = host_protocol->state == CNT_PROTOCOL_STATE_HOST_NONE || client->protocol == CNT_PROTOCOL_STATE_CLIENT_NONE;
	if (should_drop)
	{
		return NULL;
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

		cnt_log("Client (%lu) received challenge\nSend Answer to Host", client->id_on_host.index);

		client->protocol = CNT_PROTOCOL_STATE_CLIENT_ANSWER;
		return NULL;
	}
	case CNT_PROTOCOL_STATE_HOST_RESOLUTION_REJECT:
	case CNT_PROTOCOL_STATE_HOST_KICKED:
		cnt_log("Client (%lu) got kicked by Host", client->id_on_host.index);
		client->protocol = CNT_PROTOCOL_STATE_CLIENT_KICKED;
		return NULL;
	case CNT_PROTOCOL_STATE_HOST_OK:
		client->protocol = CNT_PROTOCOL_STATE_CLIENT_OK;
		break;
	}
	return client;
}

cnt_protocol_host *cnt_protocol_host_create_ok(cnt_protocol_host *host_protocol)
{
	CNT_NULL_CHECK(host_protocol);

	cnt_zerop(host_protocol);

	host_protocol->prefix = 8008;
	host_protocol->state = CNT_PROTOCOL_STATE_HOST_OK;
	return host_protocol;
}

cnt_protocol_host *cnt_protocol_host_create(cnt_protocol_host *host_protocol, cnt_host *host, cnt_client_on_host *client_state)
{
	CNT_NULL_CHECK(host);
	CNT_NULL_CHECK(client_state);
	CNT_NULL_CHECK(host_protocol);

	cnt_assert(client_state->protocol != CNT_PROTOCOL_STATE_HOST_NONE && "Client state is in invalid state. It shall not be NONE");

	cnt_zerop(host_protocol);

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

cnt_host *cnt_host_open(cnt_host *host, fckc_u32 max_connections)
{
	cnt_assert(host && "Host is null pointer");

	cnt_sparse_list_open(&host->mapping, max_connections);

	host->client_states = (cnt_client_on_host *)SDL_malloc(sizeof(*host->client_states) * max_connections);
	host->ip_lookup = (cnt_ip *)SDL_malloc(sizeof(*host->ip_lookup) * max_connections);

	return host;
}

int cnt_host_is_client_connected(cnt_host *host, cnt_sparse_index client_id)
{
	cnt_assert(host && "Host is null pointer");

	cnt_dense_index dense_index;
	if (!cnt_sparse_list_try_get_dense(&host->mapping, &client_id, &dense_index))
	{
		return 0;
	}
	cnt_client_on_host *client_on_host = &host->client_states[dense_index.index];
	return client_on_host->protocol == CNT_PROTOCOL_STATE_HOST_OK;
}

int cnt_host_client_ip_try_get(cnt_host *host, cnt_sparse_index client_id, cnt_ip *ip)
{
	cnt_assert(host && "Host is null pointer");
	cnt_assert(ip && "IP is null pointer");

	cnt_dense_index dense_index;
	if (!cnt_sparse_list_try_get_dense(&host->mapping, &client_id, &dense_index))
	{
		return 0;
	}
	cnt_memcpy(ip, &host->ip_lookup[dense_index.index], sizeof(*ip));
	return 1;
}

int cnt_client_on_host_should_disconnect(cnt_client_on_host *client)
{
	return client->protocol == CNT_PROTOCOL_STATE_HOST_NONE || client->protocol == CNT_PROTOCOL_STATE_HOST_DISCONNECT;
}

cnt_host *cnt_host_send(cnt_host *host, cnt_stream *stream, cnt_ip_container *container, cnt_message_64_bytes_queue *messages)
{
	cnt_assert(host && "Host is null pointer");

	host->send_count = host->send_count + 1;

	// All ok clients get a thin host protocol telling them everything is fine
	cnt_protocol_host host_protocol;
	cnt_protocol_host_create_ok(&host_protocol);
	cnt_protocol_host_write(&host_protocol, stream);

	// TODO: Make config
	const fckc_u8 MAXIMUM_HANDSHAKE_ATTEMPTS = 64;

	// The container declares who we send shit to
	cnt_ip_container_clear(container);

	// Clean up clients stuck in handshake or clients with a faulty protocol
	cnt_sparse_list *mapping = &host->mapping;
	for (fckc_u32 index = 0; index < mapping->count; index++)
	{
		cnt_dense_index dense_index{index};
		cnt_sparse_index sparse_index;
		int has_sparse = cnt_sparse_list_try_get_sparse(mapping, &dense_index, &sparse_index);
		cnt_assert(has_sparse && "Sparse index does not exist for dense index");

		cnt_client_on_host *client_state = &host->client_states[dense_index.index];
		cnt_assert(client_state->id.index == sparse_index.index && "Sparse Index does not align");

		cnt_protocol_host protocol;
		// NOTE: Maybe there are better conditions to check this...
		if (cnt_client_on_host_should_disconnect(client_state) || client_state->attempts > MAXIMUM_HANDSHAKE_ATTEMPTS)
		{
			// We get the last index BEFORE we remove.
			// Only for consistency so last is count - 1
			fckc_u32 last = mapping->count - 1;

			cnt_dense_index dirty_dense;
			int has_dense = cnt_sparse_list_try_get_dense(mapping, &client_state->id, &dirty_dense);
			cnt_assert(has_dense && dirty_dense.index == dense_index.index && "Dense index does not exist for sparse index");

			int remove_result = cnt_sparse_list_remove(mapping, &client_state->id);
			cnt_assert(remove_result && "Removal of connection id was unsuccessful");

			// If the asserts above fail we are in a critical state :( Could create a bruteforce approach in the long run to recover
			cnt_memcpy(&host->ip_lookup[dirty_dense.index], &host->ip_lookup[last], sizeof(*host->ip_lookup));
			cnt_memcpy(&host->client_states[dirty_dense.index], &host->client_states[last], sizeof(*host->client_states));

			// Null out last
			cnt_zerop(host->ip_lookup + last);
			cnt_zerop(host->client_states + last);

			// We want to repeat index since we removed 1 from count and moved last into current
			index = index - 1;
		}
	}

	for (fckc_u32 index = 0; index < mapping->count; index++)
	{
		cnt_dense_index dense_index = {index};
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
				cnt_memcpy(&message.ip, ip, sizeof(message.ip));

				cnt_stream protocol_stream;
				cnt_stream_create(&protocol_stream, message.payload, sizeof(message.payload));

				cnt_protocol_host_write(&protocol, &protocol_stream);
				message.payload_count = protocol_stream.at;
				cnt_message_queue_64_bytes_push(messages, &message);
			}
		}
		// else
		//{
		//  Whenever we send something to a client, we increment the attempts by 1
		//  Whenever we recv something, we set the client's attempt counter to 0
		//  We do this cause we want to be indipendent from a frequency.
		//  We want to make sure that we stay within a margin of error.
		//  If we allow 100 attempts, and we consistently reach a attempt count of
		//  75, we can confidently 75% of the packets are getting lost.
		//  We can further more optimise this approach by calculating a frequency requirement
		//  on top of the attempts counter, so attempts 0 to 10 map to 10ms and 90 to 100 map to 250ms
		client_state->attempts = client_state->attempts + 1;
		//}
	}

	return host;
}

cnt_client_on_host *cnt_host_recv(cnt_host *host, cnt_ip *client_addr, cnt_stream *stream)
{
	cnt_assert(host && "Host is null pointer");

	host->recv_count = host->recv_count + 1;

	cnt_protocol_client protocol;
	if (cnt_protocol_client_read(&protocol, stream))
	{
		return cnt_protocol_client_apply(&protocol, host, client_addr);
	}
	return NULL;
}

int cnt_host_kick(cnt_host *host, cnt_sparse_index client_id)
{
	cnt_assert(host && "Host is null pointer");

	cnt_dense_index dense_index;
	if (cnt_sparse_list_try_get_dense(&host->mapping, &client_id, &dense_index))
	{
		cnt_log("Host kicks Client(%lu)", client_id.index);

		cnt_client_on_host *state = &host->client_states[dense_index.index];
		state->protocol = CNT_PROTOCOL_STATE_HOST_KICKED;
		return 1;
	}

	return 0;
}

void cnt_host_close(cnt_host *host)
{
	cnt_assert(host && "Host is null pointer");

	SDL_free(host->ip_lookup);
	SDL_free(host->client_states);
	cnt_sparse_list_close(&host->mapping);

	cnt_zerop(host);
}

cnt_client *cnt_client_open(cnt_client *client, cnt_connection *connection)
{
	cnt_assert(client && "Client is null pointer");
	cnt_assert(connection && "Connection is null pointer");

	cnt_zerop(client);

	cnt_connection_open(&client->connection, &connection->src, &connection->dst);
	client->id_on_host.index = CNT_SPARSE_INDEX_INVALID;
	client->protocol = CNT_PROTOCOL_STATE_CLIENT_REQUEST;

	return client;
}

cnt_client *cnt_client_send(cnt_client *client, cnt_stream *stream)
{
	CNT_NULL_CHECK(client);
	CNT_NULL_CHECK(stream);
	// cnt_assert(client->protocol != CNT_PROTOCOL_STATE_CLIENT_NONE);

	const fckc_u8 MAXIMUM_HANDSHAKE_ATTEMPTS = 64;

	if (client->attempts > MAXIMUM_HANDSHAKE_ATTEMPTS)
	{
		client->protocol = CNT_PROTOCOL_STATE_CLIENT_NONE; // Maybe timeout instead?
		// Timeout or invalid state - Nothing can be done
		return NULL;
	}

	// Preferably prefer a message!
	cnt_protocol_client protocol;
	if (!cnt_protocol_client_create(&protocol, client))
	{
		// Failed to follow protocol - Nothing can be done
		return NULL;
	}

	// Checking this might be redundant...
	if (client->host_protocol > CNT_PROTOCOL_STATE_HOST_OK)
	{
		// Host booted us :(
		return NULL;
	}
	cnt_protocol_client_write(&protocol, stream);

	if (client->protocol != CNT_PROTOCOL_STATE_CLIENT_OK)
	{
		// Work on handshake
		return NULL;
	}

	// Normal flow
	return client;
}

void cnt_client_disconnect(cnt_client *client)
{
	client->protocol = CNT_PROTOCOL_STATE_CLIENT_DISCONNECT;
}

int cnt_client_is_active(cnt_client *client)
{
	return client->protocol != CNT_PROTOCOL_STATE_CLIENT_NONE && client->protocol != CNT_PROTOCOL_STATE_CLIENT_KICKED;
}

cnt_client *cnt_client_recv(cnt_client *client, cnt_stream *stream)
{
	CNT_NULL_CHECK(client);
	CNT_NULL_CHECK(stream);

	cnt_protocol_host protocol;
	if (cnt_protocol_host_read(&protocol, stream))
	{
		return cnt_protocol_host_apply(&protocol, client);
	}
	return NULL;
}

void cnt_client_close(cnt_client *client)
{
	cnt_assert(client && "Client is null pointer");

	cnt_connection_close(&client->connection);

	cnt_zerop(client);
}

cnt_user_client_frame *cnt_user_client_frame_alloc(cnt_stream *stream)
{
	CNT_NULL_CHECK(stream);
	cnt_assert(stream->at >= 0);

	// Heap allocation for this one is very stupid
	const fckc_u32 size = offsetof(cnt_user_client_frame, data[stream->at]);
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

cnt_list_header *cnt_list_header_open(cnt_list_header *header, fckc_u32 capacity)
{
	CNT_NULL_CHECK(header);
	header->capacity = capacity;
	header->count = 0;
	return header;
}

void cnt_list_header_close(cnt_list_header *header)
{
	CNT_NULL_CHECK(header);

	cnt_zerop(header);
}

void cnt_list_header_clear(cnt_list_header *header)
{
	CNT_NULL_CHECK(header);
	header->count = 0;
}

void cnt_list_header_assert_will_not_overflow(cnt_list_header *header, fckc_u32 count)
{
	CNT_NULL_CHECK(header);

	cnt_assert(header->count + count <= header->capacity);
}

int cnt_list_header_add(cnt_list_header *header, void *data, size_t stride, void *element, fckc_u32 count)
{
	CNT_NULL_CHECK(header);

	cnt_list_header_assert_will_not_overflow(header, count);

	fckc_u32 at = header->count;
	cnt_memcpy(((fckc_u8 *)data) + at * stride * count, element, stride * count);

	header->count = header->count + count;

	return 1;
}

cnt_user_host_client_list *cnt_user_host_client_list_alloc(fckc_u32 capacity)
{
	const fckc_u32 size = offsetof(cnt_user_host_client_list, clients[capacity]);
	cnt_user_host_client_list *list = (cnt_user_host_client_list *)SDL_malloc(size);
	cnt_list_header_open(&list->header, capacity);
	return list;
}

void cnt_user_host_client_list_free(cnt_user_host_client_list *list)
{
	CNT_NULL_CHECK(list);
	cnt_list_header_close(&list->header);
	SDL_free(list);
}

void cnt_user_host_client_list_clear(cnt_user_host_client_list *list)
{
	CNT_NULL_CHECK(list);
	cnt_list_header_clear(&list->header);
}

void cnt_user_host_client_list_add(cnt_user_host_client_list *list, cnt_client_on_host *clients, fckc_u32 count)
{
	CNT_NULL_CHECK(list);
	if (count == 0)
	{
		return;
	}
	cnt_list_header_add(&list->header, list->clients, sizeof(*clients), clients, count);
}

cnt_user_host_client_spsc_list *cnt_user_host_client_spsc_list_open(cnt_user_host_client_spsc_list *list, fckc_u32 capacity)
{
	CNT_NULL_CHECK(list);

	cnt_zerop(list);

	list->capacity = capacity;

	list->lists[0] = cnt_user_host_client_list_alloc(capacity);
	list->lists[1] = cnt_user_host_client_list_alloc(capacity);
	return list;
}

void cnt_user_host_client_spsc_list_close(cnt_user_host_client_spsc_list *list)
{
	CNT_NULL_CHECK(list);

	cnt_user_host_client_list_free(list->lists[0]);
	cnt_user_host_client_list_free(list->lists[1]);

	cnt_zerop(list);
}

void cnt_user_host_client_spsc_list_clear(cnt_user_host_client_spsc_list *list)
{
	CNT_NULL_CHECK(list);
	cnt_user_host_client_list *current_list = list->lists[list->current_inactive];
	cnt_user_host_client_list_clear(current_list);
}

void cnt_user_host_client_spsc_list_add(cnt_user_host_client_spsc_list *list, cnt_client_on_host *clients, fckc_u32 count)
{
	CNT_NULL_CHECK(list);

	cnt_user_host_client_list *current_list = list->lists[list->current_inactive];
	cnt_user_host_client_list_add(current_list, clients, count);
}

void cnt_user_host_client_spsc_list_lock(cnt_user_host_client_spsc_list *list)
{
	CNT_NULL_CHECK(list);
	while (SDL_CompareAndSwapAtomicU32(&list->lock, 0, 1))
	{
	}
}

void cnt_user_host_client_spsc_list_unlock(cnt_user_host_client_spsc_list *list)
{
	CNT_NULL_CHECK(list);

	SDL_SetAtomicU32(&list->lock, 0);
}

cnt_client_on_host *cnt_user_host_client_spsc_list_get(cnt_user_host_client_spsc_list *list, fckc_u32 *count)
{
	CNT_NULL_CHECK(list);

	cnt_user_host_client_list *current_list = (cnt_user_host_client_list *)SDL_GetAtomicPointer((void **)&list->active);
	if (current_list == NULL)
	{
		*count = 0;
		return NULL;
	}
	*count = current_list->header.count;
	return current_list->clients;
}

int cnt_user_host_client_spsc_list_submit(cnt_user_host_client_spsc_list *list)
{
	CNT_NULL_CHECK(list);

	if (SDL_CompareAndSwapAtomicU32(&list->lock, 0, 1))
	{
		cnt_user_host_client_list *current_list = list->lists[list->current_inactive];
		SDL_SetAtomicPointer((void **)&list->active, current_list);

		// flip flop between the inactive queues when user does not have any
		list->current_inactive = (list->current_inactive + 1) % 2;

		SDL_SetAtomicU32(&list->lock, 0);
		return 1;
	}
	return 0;
}

cnt_queue_header *cnt_queue_header_open(cnt_queue_header *header, fckc_u32 capacity)
{
	CNT_NULL_CHECK(header);
	header->capacity = capacity;
	header->head = 0;
	header->tail = 0;
	return header;
}

fckc_u32 cnt_queue_header_count(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	return (header->tail - header->head + header->capacity) % header->capacity;
}

// TODO: Make this return 0/1 instead of asserting
// Server will be exploitable otherwise by bombarding it!
void cnt_queue_header_assert_is_not_full(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	fckc_u32 next = (header->tail + 1) % header->capacity;

	cnt_assert(next != header->head);
}

int cnt_queue_header_is_empty(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	return header->head == header->tail;
}

fckc_u32 cnt_queue_header_head(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	return header->head;
}

fckc_u32 cnt_queue_header_tail(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	return header->tail;
}

fckc_u32 cnt_queue_header_advance_head(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	fckc_u32 index = cnt_queue_header_head(header);

	header->head = (header->head + 1) % header->capacity;
	return index;
}

fckc_u32 cnt_queue_header_advance_tail(cnt_queue_header *header)
{
	CNT_NULL_CHECK(header);

	fckc_u32 index = cnt_queue_header_tail(header);

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

	cnt_zerop(header);
}

int cnt_queue_header_try_enqueue(cnt_queue_header *header, void *data, size_t stride, void *element)
{
	CNT_NULL_CHECK(header);

	// try_enqueue 0 not implemented

	cnt_queue_header_assert_is_not_full(header);

	fckc_u32 tail = cnt_queue_header_advance_tail(header);
	cnt_memcpy(((fckc_u8 *)data) + tail * stride, element, stride);

	return 1;
}

int cnt_queue_header_try_peek(cnt_queue_header *header, void *data, size_t stride, void *element)
{
	CNT_NULL_CHECK(header);

	if (cnt_queue_header_is_empty(header))
	{
		return 0;
	}

	fckc_u32 head = cnt_queue_header_head(header);
	fckc_u8 *data_at = ((fckc_u8 *)data) + head * stride;
	cnt_memcpy(element, data_at, stride);

	return 1;
}

int cnt_queue_header_try_dequeue(cnt_queue_header *header, void *data, size_t stride, void *element)
{
	CNT_NULL_CHECK(header);

	if (cnt_queue_header_is_empty(header))
	{
		return 0;
	}

	fckc_u32 head = cnt_queue_header_advance_head(header);
	fckc_u8 *data_at = ((fckc_u8 *)data) + head * stride;
	cnt_memcpy(element, data_at, stride);
	SDL_memset(data_at, 0, stride);

	return 1;
}

#define CNT_DEFINE_QUEUE(type, element_type, data_name)                                                                                    \
	type *type##_alloc(fckc_u32 capacity)                                                                                                  \
	{                                                                                                                                      \
		const fckc_u32 size = offsetof(type, data_name[capacity]);                                                                         \
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
	fckc_u32 type##_count(type *queue)                                                                                                     \
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
	int type##_is_empty(type *queue)                                                                                                       \
	{                                                                                                                                      \
		CNT_NULL_CHECK(queue);                                                                                                             \
                                                                                                                                           \
		return cnt_queue_header_is_empty(&queue->header);                                                                                  \
	}                                                                                                                                      \
                                                                                                                                           \
	int type##_try_peek(type *queue, element_type *data)                                                                                   \
	{                                                                                                                                      \
		CNT_NULL_CHECK(queue);                                                                                                             \
		CNT_NULL_CHECK(data);                                                                                                              \
                                                                                                                                           \
		return cnt_queue_header_try_peek(&queue->header, (void *)queue->data_name, sizeof(*queue->data_name), (void *)data);               \
	}                                                                                                                                      \
                                                                                                                                           \
	int type##_try_dequeue(type *queue, element_type *data)                                                                                \
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

cnt_user_client_frame_spsc_queue *cnt_user_client_frame_spsc_queue_open(cnt_user_client_frame_spsc_queue *queue, fckc_u32 capacity)
{
	CNT_NULL_CHECK(queue);

	cnt_zerop(queue);

	queue->queues[0] = cnt_user_client_frame_queue_alloc(capacity);
	queue->queues[1] = cnt_user_client_frame_queue_alloc(capacity);

	return queue;
}

void cnt_user_client_frame_spsc_queue_close(cnt_user_client_frame_spsc_queue *queue)
{
	CNT_NULL_CHECK(queue);

	cnt_user_client_frame_queue_free(queue->queues[0]);
	cnt_user_client_frame_queue_free(queue->queues[1]);

	cnt_zerop(queue);
}

cnt_user_client_frame_spsc_queue *cnt_user_client_frame_spsc_queue_enqueue(cnt_user_client_frame_spsc_queue *queue,
                                                                           cnt_user_client_frame *frame)
{
	CNT_NULL_CHECK(queue);

	cnt_user_client_frame_queue *current_queue = queue->queues[queue->current_inactive];
	cnt_user_client_frame_queue_enqueue(current_queue, frame);

	return queue;
}

cnt_user_client_frame_spsc_queue *cnt_user_client_frame_spsc_queue_submit(cnt_user_client_frame_spsc_queue *queue)
{
	CNT_NULL_CHECK(queue);

	// While we have an active queue, it implies the user is still working on previosu data
	cnt_user_client_frame_queue *active = (cnt_user_client_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == NULL)
	{
		cnt_user_client_frame_queue *current_queue = queue->queues[queue->current_inactive];
		SDL_SetAtomicPointer((void **)&queue->active, current_queue);

		// flip flop between the inactive queues when user does not have any
		queue->current_inactive = (queue->current_inactive + 1) % 2;
	}

	return queue;
}

int cnt_user_client_frame_spsc_queue_try_dequeue(cnt_user_client_frame_spsc_queue *queue, cnt_user_client_frame **frame)
{
	CNT_NULL_CHECK(queue);
	CNT_NULL_CHECK(frame);

	cnt_user_client_frame_queue *active = (cnt_user_client_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == NULL)
	{
		return 0;
	}

	if (cnt_user_client_frame_queue_try_dequeue(active, frame))
	{
		if (cnt_user_client_frame_queue_is_empty(active))
		{
			SDL_SetAtomicPointer((void **)&queue->active, NULL);
		}
		return 1;
	}

	return 0;
}

int cnt_user_client_frame_spsc_queue_try_peek(cnt_user_client_frame_spsc_queue *queue, cnt_user_client_frame **frame)
{
	CNT_NULL_CHECK(queue);
	CNT_NULL_CHECK(frame);

	cnt_user_client_frame_queue *active = (cnt_user_client_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == NULL)
	{
		return 0;
	}

	if (cnt_user_client_frame_queue_try_peek(active, frame))
	{
		return 1;
	}

	return 0;
}

cnt_user_host_frame *cnt_user_host_frame_alloc(cnt_stream *stream, cnt_sparse_index client_id)
{
	CNT_NULL_CHECK(stream);
	cnt_assert(stream->at >= 0);

	// Heap allocation for this one is very stupid
	const fckc_u32 size = offsetof(cnt_user_host_frame, data[stream->at]);
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

cnt_user_host_frame_spsc_queue *cnt_user_host_frame_spsc_queue_open(cnt_user_host_frame_spsc_queue *queue, fckc_u32 capacity)
{
	CNT_NULL_CHECK(queue);

	cnt_zerop(queue);

	queue->queues[0] = cnt_user_host_frame_queue_alloc(capacity);
	queue->queues[1] = cnt_user_host_frame_queue_alloc(capacity);

	return queue;
}

void cnt_user_host_frame_spsc_queue_close(cnt_user_host_frame_spsc_queue *queue)
{
	CNT_NULL_CHECK(queue);

	cnt_user_host_frame_queue_free(queue->queues[0]);
	cnt_user_host_frame_queue_free(queue->queues[1]);

	cnt_zerop(queue);
}

cnt_user_host_frame_spsc_queue *cnt_user_host_frame_spsc_queue_enqueue(cnt_user_host_frame_spsc_queue *queue, cnt_user_host_frame *frame)
{
	CNT_NULL_CHECK(queue);

	cnt_user_host_frame_queue *current_queue = queue->queues[queue->current_inactive];
	cnt_user_host_frame_queue_enqueue(current_queue, frame);

	return queue;
}

// Maybe commit to keep it cool
cnt_user_host_frame_spsc_queue *cnt_user_host_frame_spsc_queue_submit(cnt_user_host_frame_spsc_queue *queue)
{
	CNT_NULL_CHECK(queue);

	// While we have an active queue, it implies the user is still working on previosu data
	cnt_user_host_frame_queue *active = (cnt_user_host_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == NULL)
	{
		cnt_user_host_frame_queue *current_queue = queue->queues[queue->current_inactive];
		SDL_SetAtomicPointer((void **)&queue->active, current_queue);

		// flip flop between the inactive queues when user does not have any
		queue->current_inactive = (queue->current_inactive + 1) % 2;
	}

	return queue;
}

int cnt_user_host_frame_spsc_queue_try_dequeue(cnt_user_host_frame_spsc_queue *queue, cnt_user_host_frame **frame)
{
	CNT_NULL_CHECK(queue);
	CNT_NULL_CHECK(frame);

	cnt_user_host_frame_queue *active = (cnt_user_host_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == NULL)
	{
		return 0;
	}

	if (cnt_user_host_frame_queue_try_dequeue(active, frame))
	{
		if (cnt_user_host_frame_queue_is_empty(active))
		{
			SDL_SetAtomicPointer((void **)&queue->active, NULL);
		}
		return 1;
	}

	return 0;
}

int cnt_user_host_frame_spsc_queue_try_peek(cnt_user_host_frame_spsc_queue *queue, cnt_user_host_frame **frame)
{
	CNT_NULL_CHECK(queue);
	CNT_NULL_CHECK(frame);

	cnt_user_host_frame_queue *active = (cnt_user_host_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == NULL)
	{
		return 0;
	}

	if (cnt_user_host_frame_queue_try_peek(active, frame))
	{
		return 1;
	}

	return 0;
}

cnt_user_host_command_spsc_queue *cnt_user_host_command_spsc_queue_open(cnt_user_host_command_spsc_queue *queue, fckc_u32 capacity)
{
	CNT_NULL_CHECK(queue);

	cnt_zerop(queue);

	queue->queues[0] = cnt_user_host_command_queue_alloc(capacity);
	queue->queues[1] = cnt_user_host_command_queue_alloc(capacity);

	return queue;
}

void cnt_user_host_command_spsc_queue_close(cnt_user_host_command_spsc_queue *queue)
{
	CNT_NULL_CHECK(queue);

	cnt_user_host_command_queue_free(queue->queues[0]);
	cnt_user_host_command_queue_free(queue->queues[1]);

	cnt_zerop(queue);
}

cnt_user_host_command_spsc_queue *cnt_user_host_command_spsc_queue_enqueue(cnt_user_host_command_spsc_queue *queue,
                                                                           cnt_user_host_command *command)
{
	CNT_NULL_CHECK(queue);

	cnt_user_host_command_queue *current_queue = queue->queues[queue->current_inactive];

	// Not optimal... Maybe the generic macro for a queue was a bad idea: Change it!
	cnt_user_host_command_queue_enqueue(current_queue, *command);

	return queue;
}

cnt_user_host_command_spsc_queue *cnt_user_host_command_spsc_queue_submit(cnt_user_host_command_spsc_queue *queue)
{
	CNT_NULL_CHECK(queue);

	// While we have an active queue, it implies the user is still working on previosu data
	cnt_user_host_frame_queue *active = (cnt_user_host_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == NULL)
	{
		cnt_user_host_command_queue *current_queue = queue->queues[queue->current_inactive];
		SDL_SetAtomicPointer((void **)&queue->active, current_queue);

		// flip flop between the inactive queues when user does not have any
		queue->current_inactive = (queue->current_inactive + 1) % 2;
	}

	return queue;
}

int cnt_user_host_command_spsc_queue_try_dequeue(cnt_user_host_command_spsc_queue *queue, cnt_user_host_command *command)
{
	CNT_NULL_CHECK(queue);
	CNT_NULL_CHECK(command);

	cnt_user_host_command_queue *active = (cnt_user_host_command_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == NULL)
	{
		return 0;
	}

	if (cnt_user_host_command_queue_try_dequeue(active, command))
	{
		if (cnt_user_host_command_queue_is_empty(active))
		{
			SDL_SetAtomicPointer((void **)&queue->active, NULL);
		}
		return 1;
	}

	return 0;
}

cnt_user_client_command_spsc_queue *cnt_user_client_command_spsc_queue_open(cnt_user_client_command_spsc_queue *queue, fckc_u32 capacity)
{
	CNT_NULL_CHECK(queue);

	cnt_zerop(queue);

	queue->queues[0] = cnt_user_client_command_queue_alloc(capacity);
	queue->queues[1] = cnt_user_client_command_queue_alloc(capacity);

	return queue;
}

void cnt_user_client_command_spsc_queue_close(cnt_user_client_command_spsc_queue *queue)
{
	CNT_NULL_CHECK(queue);

	cnt_user_client_command_queue_free(queue->queues[0]);
	cnt_user_client_command_queue_free(queue->queues[1]);

	cnt_zerop(queue);
}

cnt_user_client_command_spsc_queue *cnt_user_client_command_spsc_queue_enqueue(cnt_user_client_command_spsc_queue *queue,
                                                                               cnt_user_client_command *command)
{
	CNT_NULL_CHECK(queue);

	cnt_user_client_command_queue *current_queue = queue->queues[queue->current_inactive];

	// Not optimal... Maybe the generic macro for a queue was a bad idea: Change it!
	cnt_user_client_command_queue_enqueue(current_queue, *command);

	return queue;
}

cnt_user_client_command_spsc_queue *cnt_user_client_command_spsc_queue_submit(cnt_user_client_command_spsc_queue *queue)
{
	CNT_NULL_CHECK(queue);

	// While we have an active queue, it implies the user is still working on previosu data
	cnt_user_client_frame_queue *active = (cnt_user_client_frame_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == NULL)
	{
		cnt_user_client_command_queue *current_queue = queue->queues[queue->current_inactive];
		SDL_SetAtomicPointer((void **)&queue->active, current_queue);

		// flip flop between the inactive queues when user does not have any
		queue->current_inactive = (queue->current_inactive + 1) % 2;
	}

	return queue;
}

int cnt_user_client_command_spsc_queue_try_dequeue(cnt_user_client_command_spsc_queue *queue, cnt_user_client_command *command)
{
	CNT_NULL_CHECK(queue);
	CNT_NULL_CHECK(command);

	cnt_user_client_command_queue *active = (cnt_user_client_command_queue *)SDL_GetAtomicPointer((void **)&queue->active);
	if (active == NULL)
	{
		return 0;
	}

	if (cnt_user_client_command_queue_try_dequeue(active, command))
	{
		if (cnt_user_client_command_queue_is_empty(active))
		{
			SDL_SetAtomicPointer((void **)&queue->active, NULL);
		}
		return 1;
	}

	return 0;
}

void cnt_user_frequency_set(cnt_user_frequency *freq, fckc_u32 hz)
{
	float ms_as_float = 1000.0f / SDL_min(1000, hz);
	fckc_u32 ms_as_u32 = (fckc_u32)ms_as_float;
	SDL_SetAtomicU32(&freq->ms, ms_as_u32);
}

fckc_u32 cnt_user_frequency_get(cnt_user_frequency *freq)
{
	return SDL_GetAtomicU32(&freq->ms);
}

void cnt_net_engine_state_set(cnt_net_engine_state *engine_state, cnt_net_engine_state_type state)
{
	fckc_u32 as_u32 = (fckc_u32)state;
	SDL_SetAtomicU32(&engine_state->state, state);
}

cnt_net_engine_state_type cnt_net_engine_state_get(cnt_net_engine_state *engine_state)
{
	return (cnt_net_engine_state_type)SDL_GetAtomicU32(&engine_state->state);
}

void cnt_client_id_on_host_set(cnt_client_id_on_host *client_id_on_host, cnt_sparse_index index)
{
	fckc_u32 as_u32 = (fckc_u32)index.index;
	SDL_SetAtomicU32(&client_id_on_host->id, as_u32);
}

cnt_sparse_index cnt_client_id_on_host_get(cnt_client_id_on_host *client_id_on_host)
{
	return (cnt_sparse_index){SDL_GetAtomicU32(&client_id_on_host->id)};
}

const char *cnt_net_engine_state_type_to_string(cnt_net_engine_state_type state)
{
	switch (state)
	{
	case CNT_NET_ENGINE_STATE_TYPE_NONE:
		return "NONE";
	case CNT_NET_ENGINE_STATE_TYPE_OPENING:
		return "OPENING";
	case CNT_NET_ENGINE_STATE_TYPE_RUNNING:
		return "RUNNING";
	case CNT_NET_ENGINE_STATE_TYPE_SHUTTING_DOWN:
		return "SHUTTING DOWN";
	case CNT_NET_ENGINE_STATE_TYPE_CLOSED:
		return "CLOSED";
	}
	return "UNKNOWN";
}

void cnt_client_state_set(cnt_client_state *client_state, cnt_protocol_state_client state)
{
	fckc_u32 as_u32 = (fckc_u32)state;
	SDL_SetAtomicU32(&client_state->state, state);
}

cnt_protocol_state_client cnt_client_state_get(cnt_client_state *client_state)
{
	return (cnt_protocol_state_client)SDL_GetAtomicU32(&client_state->state);
}

const char *cnt_protocol_state_client_to_string(cnt_protocol_state_client state)
{
	switch (state)
	{
	case CNT_PROTOCOL_STATE_CLIENT_NONE:
		return "NONE";
	case CNT_PROTOCOL_STATE_CLIENT_REQUEST:
		return "REQUEST";
	case CNT_PROTOCOL_STATE_CLIENT_ANSWER:
		return "ANSWER";
	case CNT_PROTOCOL_STATE_CLIENT_OK:
		return "OK";
	case CNT_PROTOCOL_STATE_CLIENT_DISCONNECT:
		return "DISCONNECT";
	case CNT_PROTOCOL_STATE_CLIENT_KICKED:
		return "KICKED";
	}
	return "UNKNOWN";
}

void cnt_host_state_set(cnt_host_state *host_state, cnt_protocol_state_host state)
{
	fckc_u32 as_u32 = (fckc_u32)state;
	SDL_SetAtomicU32(&host_state->state, state);
}

cnt_protocol_state_host cnt_host_state_get(cnt_host_state *host_state)
{
	return (cnt_protocol_state_host)SDL_GetAtomicU32(&host_state->state);
}

const char *cnt_protocol_state_host_to_string(cnt_protocol_state_host state)
{
	switch (state)
	{
	case CNT_PROTOCOL_STATE_HOST_NONE:
		return "NONE";
	case CNT_PROTOCOL_STATE_HOST_CHALLENGE:
		return "CHALLENGE";
	case CNT_PROTOCOL_STATE_HOST_OK:
		return "OK";
	case CNT_PROTOCOL_STATE_HOST_RESOLUTION_REJECT:
		return "REJECT";
	case CNT_PROTOCOL_STATE_HOST_DISCONNECT:
		return "DISCONNECTED";
	case CNT_PROTOCOL_STATE_HOST_KICKED:
		return "KICKED";
	}
	return "UNKNOWN";
}

int cnt_net_engine_state_type_can_network(cnt_net_engine_state_type state)
{
	// We allow enqueueing state while opening, this might backlash
	return state == CNT_NET_ENGINE_STATE_TYPE_RUNNING || state == CNT_NET_ENGINE_STATE_TYPE_OPENING;
}

cnt_user_client *cnt_user_client_open(cnt_user_client *user, const char *host_ip, fckc_u16 host_port, fckc_u32 frequency)
{
	CNT_NULL_CHECK(user);
	CNT_NULL_CHECK(host_ip);
	cnt_assert(host_port != 0 && "Host port - impossible to connect to host");

	cnt_zerop(user);

	// Maybe copy IP in since this will not work with matchmakers, in other words, it will be unsafe
	user->host_ip = host_ip;
	user->host_port = host_port;

	cnt_user_frequency_set(&user->frequency, frequency);

	cnt_user_client_frame_spsc_queue_open(&user->send_queue, 1024);
	cnt_user_client_frame_spsc_queue_open(&user->recv_queue, 1024);

	cnt_user_client_command_spsc_queue_open(&user->command_queue, 64);

	cnt_net_engine_state_set(&user->net_engine_state, CNT_NET_ENGINE_STATE_TYPE_USER_INITALISED);

	SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)cnt_client_default_process, "Network Client Thread", user);
	SDL_DetachThread(thread);

	return user;
}

cnt_net_engine_state_type cnt_user_client_get_state(cnt_user_client *user)
{
	return cnt_net_engine_state_get(&user->net_engine_state);
}

cnt_protocol_state_client cnt_user_client_get_client_protocol_state(cnt_user_client *user)
{
	return cnt_client_state_get(&user->protocol_state);
}

cnt_protocol_state_host cnt_user_client_get_host_protocol_state(cnt_user_client *user)
{
	return cnt_host_state_get(&user->state_on_host);
}

cnt_sparse_index cnt_user_client_get_client_id_on_host(cnt_user_client *user)
{
	return cnt_client_id_on_host_get(&user->client_id_on_host);
}

int cnt_user_client_is_active(cnt_user_client *user)
{
	cnt_net_engine_state_type state = cnt_user_client_get_state(user);
	return state != CNT_NET_ENGINE_STATE_TYPE_CLOSED && state != CNT_NET_ENGINE_STATE_TYPE_NONE;
}

const char *cnt_user_client_state_to_string(cnt_user_client *user)
{
	cnt_net_engine_state_type state = cnt_user_client_get_state(user);
	return cnt_net_engine_state_type_to_string(state);
}

const char *cnt_user_client_client_protocol_to_string(cnt_user_client *user)
{
	cnt_protocol_state_client state = cnt_user_client_get_client_protocol_state(user);
	return cnt_protocol_state_client_to_string(state);
}
const char *cnt_user_client_host_protocol_to_string(cnt_user_client *user)
{
	cnt_protocol_state_host state = cnt_user_client_get_host_protocol_state(user);
	return cnt_protocol_state_host_to_string(state);
}

cnt_user_client *cnt_user_client_shut_down(cnt_user_client *user)
{
	CNT_NULL_CHECK(user);

	if (cnt_user_client_get_state(user) == CNT_NET_ENGINE_STATE_TYPE_NONE)
	{
		return user;
	}

	cnt_log("Shutting down Client(%s:%hu)", user->host_ip, user->host_port);

	cnt_user_client_command command;
	command.type = CNT_USER_CLIENT_COMMAND_TYPE_QUIT;
	cnt_user_client_command_spsc_queue_enqueue(&user->command_queue, &command);
	cnt_user_client_command_spsc_queue_submit(&user->command_queue);

	return user;
}

void cnt_user_client_close(cnt_user_client *user)
{
	CNT_NULL_CHECK(user);

	if (cnt_user_client_get_state(user) == CNT_NET_ENGINE_STATE_TYPE_NONE)
	{
		return;
	}

	if (cnt_user_client_get_state(user) != CNT_NET_ENGINE_STATE_TYPE_CLOSED)
	{
		cnt_user_client_shut_down(user);
	}
	while (cnt_user_client_get_state(user) != CNT_NET_ENGINE_STATE_TYPE_CLOSED)
	{
		cnt_log("Waiting to close down Client(%s:%hu)", user->host_ip, user->host_port);
	}
	cnt_user_client_frame_spsc_queue_close(&user->send_queue);
	cnt_user_client_frame_spsc_queue_close(&user->recv_queue);
	cnt_user_client_command_spsc_queue_close(&user->command_queue);

	cnt_log("Closed down Client(%s:%hu)", user->host_ip, user->host_port);

	cnt_zerop(user);
}

cnt_user_host *cnt_user_host_open(cnt_user_host *user, const char *host_ip, fckc_u16 host_port, fckc_u32 max_clients, fckc_u32 frequency)
{
	CNT_NULL_CHECK(user);
	CNT_NULL_CHECK(host_ip);
	cnt_assert(host_port != 0 && "Host port - impossible to connect to host");

	cnt_zerop(user);

	// Maybe copy IP in since this will not work with matchmakers, in other words, it will be unsafe
	user->host_ip = host_ip;
	user->host_port = host_port;
	user->max_clients = max_clients;
	cnt_user_frequency_set(&user->frequency, frequency);

	cnt_user_host_frame_spsc_queue_open(&user->send_queue, 1024);
	cnt_user_host_frame_spsc_queue_open(&user->recv_queue, 1024);

	cnt_user_host_client_spsc_list_open(&user->client_list, max_clients);

	const fckc_u32 messages_per_client = 2;
	cnt_user_host_command_spsc_queue_open(&user->command_queue, max_clients * messages_per_client);

	cnt_net_engine_state_set(&user->net_engine_state, CNT_NET_ENGINE_STATE_TYPE_USER_INITALISED);

	SDL_Thread *thread = SDL_CreateThread((SDL_ThreadFunction)cnt_host_default_process, "Network Host Thread", user);
	SDL_DetachThread(thread);

	return user;
}

cnt_user_host *cnt_user_host_shut_down(cnt_user_host *user)
{
	CNT_NULL_CHECK(user);

	if (cnt_user_host_get_state(user) == CNT_NET_ENGINE_STATE_TYPE_NONE)
	{
		return user;
	}

	cnt_log("Shutting down Host(%s:%hu)", user->host_ip, user->host_port);

	cnt_user_host_command command;
	command.type = CNT_USER_HOST_COMMAND_TYPE_QUIT;
	cnt_user_host_command_spsc_queue_enqueue(&user->command_queue, &command);
	cnt_user_host_command_spsc_queue_submit(&user->command_queue);

	return user;
}

cnt_net_engine_state_type cnt_user_host_get_state(cnt_user_host *user)
{
	return cnt_net_engine_state_get(&user->net_engine_state);
}

int cnt_user_host_is_active(cnt_user_host *user)
{
	cnt_net_engine_state_type state = cnt_user_host_get_state(user);
	return state != CNT_NET_ENGINE_STATE_TYPE_CLOSED && state != CNT_NET_ENGINE_STATE_TYPE_NONE;
}

const char *cnt_user_host_state_to_string(cnt_user_host *user)
{
	cnt_net_engine_state_type state = cnt_user_host_get_state(user);
	return cnt_net_engine_state_type_to_string(state);
}

void cnt_user_host_close(cnt_user_host *user)
{
	CNT_NULL_CHECK(user);

	if (cnt_user_host_get_state(user) == CNT_NET_ENGINE_STATE_TYPE_NONE)
	{
		return;
	}

	if (cnt_user_host_get_state(user) != CNT_NET_ENGINE_STATE_TYPE_CLOSED)
	{
		cnt_user_host_shut_down(user);
	}
	while (cnt_user_host_get_state(user) != CNT_NET_ENGINE_STATE_TYPE_CLOSED)
	{
		cnt_log("Waiting to close down Host(%s:%hu)", user->host_ip, user->host_port);
	}
	// TODO: Exhaust queue
	cnt_user_host_frame_spsc_queue_close(&user->send_queue);
	cnt_user_host_frame_spsc_queue_close(&user->recv_queue);
	cnt_user_host_command_spsc_queue_close(&user->command_queue);
	cnt_user_host_client_spsc_list_close(&user->client_list);

	cnt_log("Closed down Host(%s:%hu)", user->host_ip, user->host_port);

	cnt_zerop(user);
}

cnt_user_client *cnt_user_client_send(cnt_user_client *client, void *ptr, int byte_count)
{
	CNT_NULL_CHECK(client);

	cnt_assert(ptr != NULL || ptr == NULL && byte_count == 0);

	cnt_net_engine_state_type state = cnt_user_client_get_state(client);
	if (!cnt_net_engine_state_type_can_network(state))
	{
		return NULL;
	}
	cnt_stream example_stream;
	cnt_stream_create_full(&example_stream, (fckc_u8 *)ptr, byte_count);
	cnt_user_client_frame *frame = cnt_user_client_frame_alloc(&example_stream);
	cnt_user_client_frame_spsc_queue_enqueue(&client->send_queue, frame);
	cnt_user_client_frame_spsc_queue_submit(&client->send_queue);
	return client;
}

cnt_user_client *cnt_user_client_keep_alive(cnt_user_client *client)
{
	CNT_NULL_CHECK(client);

	cnt_user_client_send(client, NULL, 0);

	return client;
}

int cnt_user_client_recv(cnt_user_client *client, void *ptr, int byte_count)
{
	CNT_NULL_CHECK(client);
	CNT_NULL_CHECK(ptr);

	cnt_net_engine_state_type state = cnt_user_client_get_state(client);
	if (!cnt_net_engine_state_type_can_network(state))
	{
		return 0;
	}

	cnt_user_client_frame *frame;
	if (!cnt_user_client_frame_spsc_queue_try_peek(&client->recv_queue, &frame))
	{
		return 0;
	}
	if (frame->count > byte_count)
	{
		// Might make sense to give an entry point to read a partial frame
		return -1;
	}
	int has_frame = cnt_user_client_frame_spsc_queue_try_dequeue(&client->recv_queue, &frame);
	cnt_assert(has_frame);

	cnt_memcpy(ptr, frame->data, frame->count);

	int result = frame->count;
	cnt_user_client_frame_free(frame);

	return result;
}

cnt_user_host *cnt_user_host_broadcast(cnt_user_host *host, void *ptr, int byte_count)
{
	CNT_NULL_CHECK(host);

	cnt_assert(ptr != NULL || ptr == NULL && byte_count == 0);

	cnt_net_engine_state_type state = cnt_user_host_get_state(host);
	if (!cnt_net_engine_state_type_can_network(state))
	{
		return NULL;
	}

	cnt_stream example_stream;
	cnt_stream_create_full(&example_stream, (fckc_u8 *)ptr, byte_count);
	// TODO: Fix frame having redundant field - make host frame and client frame separate concepts!
	cnt_user_host_frame *frame = cnt_user_host_frame_alloc(&example_stream, (cnt_sparse_index){CNT_SPARSE_INDEX_INVALID});
	cnt_user_host_frame_spsc_queue_enqueue(&host->send_queue, frame);
	cnt_user_host_frame_spsc_queue_submit(&host->send_queue);
	return host;
}

cnt_user_host *cnt_user_host_keep_alive(cnt_user_host *host)
{
	CNT_NULL_CHECK(host);

	cnt_user_host_broadcast(host, NULL, 0);

	return host;
}

cnt_user_host *cnt_user_host_send(cnt_user_host *host, cnt_sparse_index client_id, void *ptr, int byte_count)
{
	CNT_NULL_CHECK(host);
	CNT_NULL_CHECK(ptr);

	cnt_net_engine_state_type state = cnt_user_host_get_state(host);
	if (!cnt_net_engine_state_type_can_network(state))
	{
		return NULL;
	}

	cnt_stream example_stream;
	cnt_stream_create_full(&example_stream, (fckc_u8 *)ptr, byte_count);
	// TODO: Fix frame having redundant field - make host frame and client frame separate concepts!
	cnt_user_host_frame *frame = cnt_user_host_frame_alloc(&example_stream, client_id);
	cnt_user_host_frame_spsc_queue_enqueue(&host->send_queue, frame);
	cnt_user_host_frame_spsc_queue_submit(&host->send_queue);
	return host;
}

cnt_user_host *cnt_user_host_kick(cnt_user_host *host, cnt_sparse_index client_id)
{
	CNT_NULL_CHECK(host);

	cnt_net_engine_state_type state = cnt_user_host_get_state(host);
	if (!cnt_net_engine_state_type_can_network(state))
	{
		return NULL;
	}

	cnt_user_host_command command;
	command.type = CNT_USER_HOST_COMMAND_TYPE_KICK;
	command.kick.client = client_id;
	cnt_user_host_command_spsc_queue_enqueue(&host->command_queue, &command);
	cnt_user_host_command_spsc_queue_submit(&host->command_queue);
	return host;
}

int cnt_user_host_recv(cnt_user_host *host, cnt_sparse_index *client_id, void *ptr, int byte_count)
{
	CNT_NULL_CHECK(host);
	CNT_NULL_CHECK(ptr);

	cnt_net_engine_state_type state = cnt_user_host_get_state(host);
	if (!cnt_net_engine_state_type_can_network(state))
	{
		return 0;
	}

	cnt_user_host_frame *frame;
	if (!cnt_user_host_frame_spsc_queue_try_peek(&host->recv_queue, &frame))
	{
		return 0;
	}
	if (frame->count > byte_count)
	{
		// Might make sense to give an entry point to read a partial frame
		return -1;
	}
	int has_frame = cnt_user_host_frame_spsc_queue_try_dequeue(&host->recv_queue, &frame);
	cnt_assert(has_frame);

	cnt_memcpy(client_id, &frame->client_id, sizeof(*client_id));
	cnt_memcpy(ptr, frame->data, frame->count);

	int result = frame->count;
	cnt_user_host_frame_free(frame);

	return result;
}

void cnt_user_host_client_list_lock(cnt_user_host *host)
{
	CNT_NULL_CHECK(host);

	cnt_user_host_client_spsc_list_lock(&host->client_list);
}

void cnt_user_host_client_list_unlock(cnt_user_host *host)
{
	CNT_NULL_CHECK(host);

	cnt_user_host_client_spsc_list_unlock(&host->client_list);
}

cnt_client_on_host *cnt_user_host_client_list_get(cnt_user_host *host, fckc_u32 *count)
{
	CNT_NULL_CHECK(host);

	return cnt_user_host_client_spsc_list_get(&host->client_list, count);
}

int cnt_client_default_process(cnt_user_client *user_client)
{
	cnt_net_engine_state_set(&user_client->net_engine_state, CNT_NET_ENGINE_STATE_TYPE_OPENING);

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

	cnt_net_engine_state_set(&user_client->net_engine_state, CNT_NET_ENGINE_STATE_TYPE_RUNNING);

	fckc_u32 accumulator = 0;

	// Tick
	while (1)
	{
		fckc_u64 start = SDL_GetTicks();

		cnt_client_state_set(&user_client->protocol_state, client.protocol);
		cnt_host_state_set(&user_client->state_on_host, client.host_protocol);

		cnt_user_client_command user_command;
		while (cnt_user_client_command_spsc_queue_try_dequeue(&user_client->command_queue, &user_command))
		{
			switch (user_command.type)
			{
			case CNT_USER_CLIENT_COMMAND_TYPE_QUIT:
				cnt_client_disconnect(&client);
				break;
			case CNT_USER_CLIENT_COMMAND_TYPE_RESTART:
				// TODO: This shit will be complex lol
				break;
			}
		}

		if (!cnt_client_is_active(&client))
		{
			goto quit;
		}

		fckc_u32 freq_ms = cnt_user_frequency_get(&user_client->frequency);
		while (accumulator >= freq_ms)
		{
			accumulator = accumulator - freq_ms;

			cnt_stream_clear(&transport.stream);
			if (cnt_client_send(&client, &transport.stream))
			{
				int transport_end = transport.stream.at;

				cnt_user_client_frame *frame;
				while (cnt_user_client_frame_spsc_queue_try_dequeue(&user_client->send_queue, &frame))
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
				if (frame_stream.capacity > 0)
				{
					cnt_user_client_frame *frame = cnt_user_client_frame_alloc(&frame_stream);
					cnt_user_client_frame_spsc_queue_enqueue(&user_client->recv_queue, frame);
					cnt_user_client_frame_spsc_queue_submit(&user_client->recv_queue);
				}
			}
		}

		cnt_client_id_on_host_set(&user_client->client_id_on_host, client.id_on_host);

		// skip_networking:
		fckc_u64 end = SDL_GetTicks();
		fckc_u64 delta = end - start;

		accumulator = accumulator + delta;

		// Feeling a bit clever, unsigned wraps around and becomes large
		// so we just always get the smaller value of the two
		// freq_ms = SDL_min(freq_ms - delta, freq_ms);
		// SDL_Delay(freq_ms);
	}

quit:
	cnt_net_engine_state_set(&user_client->net_engine_state, CNT_NET_ENGINE_STATE_TYPE_SHUTTING_DOWN);

	// Cleanup
	cnt_stream_close(&stream);

	cnt_client_close(&client);

	cnt_compression_close(&compression);

	cnt_transport_close(&transport);

	cnt_connection_close(&connection);

	cnt_sock_close(&client_socket);

	cnt_tead_down();

	cnt_net_engine_state_set(&user_client->net_engine_state, CNT_NET_ENGINE_STATE_TYPE_CLOSED);

	return 0;
}

int cnt_host_default_process(cnt_user_host *user_host)
{
	cnt_net_engine_state_set(&user_host->net_engine_state, CNT_NET_ENGINE_STATE_TYPE_OPENING);

	cnt_start_up();

	// Startup
	cnt_sock server_socket;
	cnt_sock_open(&server_socket, user_host->host_ip, user_host->host_port);

	cnt_star star;
	cnt_star_open(&star, &server_socket, user_host->max_clients);

	cnt_host host;
	cnt_host_open(&host, user_host->max_clients);

	cnt_message_64_bytes_queue message_queue;
	cnt_message_queue_64_bytes_open(&message_queue, 128);

	cnt_transport transport;
	cnt_transport_open(&transport, 1 << 16);

	cnt_compression compression;
	cnt_compression_open(&compression, 1 << 16);

	cnt_net_engine_state_set(&user_host->net_engine_state, CNT_NET_ENGINE_STATE_TYPE_RUNNING);

	fckc_u32 accumulator = 0;
	// Tick
	while (1)
	{
		fckc_u64 start = SDL_GetTicks();

		cnt_user_host_command user_command;
		while (cnt_user_host_command_spsc_queue_try_dequeue(&user_host->command_queue, &user_command))
		{
			switch (user_command.type)
			{
			case CNT_USER_HOST_COMMAND_TYPE_QUIT:
				goto quit;
			case CNT_USER_HOST_COMMAND_TYPE_RESTART:
				// TODO: This shit will be complex lol
				// For now we could just turn it off and on again :D
				break;
			case CNT_USER_HOST_COMMAND_TYPE_KICK: {
				cnt_sparse_index client = user_command.kick.client;
				cnt_host_kick(&host, client);
				break;
			}
			}
		}

		fckc_u32 freq_ms = cnt_user_frequency_get(&user_host->frequency);
		while (accumulator >= freq_ms)
		{
			accumulator = accumulator - freq_ms;
			// Clear the message queue since we receive a new queue from host layer
			cnt_message_queue_64_bytes_clear(&message_queue);

			// Adds header to packet
			cnt_stream_clear(&transport.stream);
			cnt_host_send(&host, &transport.stream, &star.destinations, &message_queue);

			int transport_end = transport.stream.at;

			cnt_user_host_frame *frame;
			while (cnt_user_host_frame_spsc_queue_try_dequeue(&user_host->send_queue, &frame))
			{
				transport.stream.at = transport_end;
				// Add data to transport
				// Send out packet - no target client_id implies broadcast
				if (frame->client_id.index != CNT_SPARSE_INDEX_INVALID)
				{
					if (cnt_host_is_client_connected(&host, frame->client_id))
					{
						cnt_stream_write_string(&transport.stream, frame->data, frame->count);
						cnt_compress(&compression, &transport.stream);

						cnt_ip ip;
						int has_ip = cnt_host_client_ip_try_get(&host, frame->client_id, &ip);
						cnt_assert(has_ip && "IP does not exist, but state is valid");

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
			for (fckc_u32 index = 0; index < message_queue.count; index++)
			{
				cnt_message_64_bytes *message = &message_queue.messages[index];
				cnt_connection_set_destination(&message_connection, &message->ip);

				cnt_stream message_stream;
				cnt_stream_create_full(&message_stream, message->payload, message->payload_count);

				cnt_compress(&compression, &message_stream);
				cnt_connection_send(&message_connection, &compression.stream);
			}
		}

		cnt_ip recv_addr;
		while (cnt_star_recv(&star, &recv_addr, &compression.stream))
		{
			cnt_decompress(&compression, &transport.stream);

			cnt_stream recv_stream;
			cnt_stream_create(&recv_stream, transport.stream.data, transport.stream.at);

			cnt_client_on_host *client = cnt_host_recv(&host, &recv_addr, &recv_stream);
			if (client != NULL)
			{
				cnt_stream frame_stream;
				cnt_stream_create_full(&frame_stream, recv_stream.data + recv_stream.at, recv_stream.capacity - recv_stream.at);
				// We probably need to differ between client frame and host frame...
				// Client KNOWS where it receives the data from
				// Host needs to propagate!
				if (frame_stream.capacity > 0)
				{
					cnt_user_host_frame *frame = cnt_user_host_frame_alloc(&frame_stream, client->id);
					cnt_user_host_frame_spsc_queue_enqueue(&user_host->recv_queue, frame);
					cnt_user_host_frame_spsc_queue_submit(&user_host->recv_queue);
				}
			}
		}

		cnt_user_host_client_spsc_list_clear(&user_host->client_list);
		cnt_user_host_client_spsc_list_add(&user_host->client_list, host.client_states, host.mapping.count);
		cnt_user_host_client_spsc_list_submit(&user_host->client_list);

		fckc_u64 end = SDL_GetTicks();
		fckc_u64 delta = end - start;

		accumulator = accumulator + delta;
		// Feeling a bit clever, unsigned wraps around and becomes large
		// so we just always get the smaller value of the two
		// freq_ms = SDL_min(freq_ms - delta, freq_ms);
	}

quit:
	cnt_net_engine_state_set(&user_host->net_engine_state, CNT_NET_ENGINE_STATE_TYPE_SHUTTING_DOWN);

	// Cleanup
	cnt_compression_close(&compression);

	cnt_transport_close(&transport);

	cnt_message_queue_64_bytes_close(&message_queue);

	cnt_host_close(&host);

	cnt_star_close(&star);

	cnt_sock_close(&server_socket);

	cnt_tead_down();

	cnt_net_engine_state_set(&user_host->net_engine_state, CNT_NET_ENGINE_STATE_TYPE_CLOSED);

	return 0;
}
