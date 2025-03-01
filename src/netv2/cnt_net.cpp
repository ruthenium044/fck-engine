#include "cnt_net.h"

#include <SDL3/SDL_stdinc.h>
#include <errno.h>
#include <string.h>

void cnt_client_on_host_mapping_open(cnt_client_on_host_mapping *mapping, uint32_t capacity)
{
	const uint32_t maximum_possible_capacity = (UINT32_MAX >> 1);

	CNT_NULL_CHECK(mapping);
	SDL_assert(capacity <= maximum_possible_capacity && "Capacity is too large. Stay below 0x7FFFFFFF");

	SDL_zerop(mapping);

	mapping->capacity = capacity;
	mapping->free_head = UINT32_MAX;
	mapping->control_bit_mask = (UINT32_MAX >> 1) + 1;

	mapping->sparse = (cnt_client_on_host_to_dense_index *)SDL_malloc(sizeof(*mapping->sparse) * mapping->capacity);
	mapping->dense = (cnt_client_on_host_index *)SDL_malloc(sizeof(*mapping->dense) * mapping->capacity);
	SDL_memset4(mapping->sparse, UINT32_MAX, sizeof(*mapping->sparse) * mapping->capacity);
}

void cnt_client_on_host_mapping_close(cnt_client_on_host_mapping *mapping)
{
	CNT_NULL_CHECK(mapping);

	SDL_free(mapping->sparse);
	SDL_free(mapping->dense);

	SDL_zerop(mapping);
}

void cnt_client_on_host_mapping_make_invalid(cnt_client_on_host_mapping *mapping, cnt_client_on_host_to_dense_index *index)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(index);

	const uint32_t control_bit_mask = mapping->control_bit_mask;
	index->index = index->index | control_bit_mask;
}

void cnt_client_on_host_mapping_make_valid(cnt_client_on_host_mapping *mapping, cnt_client_on_host_to_dense_index *index)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(index);

	const uint32_t control_bit_mask = mapping->control_bit_mask;
	index->index = (index->index & ~control_bit_mask);
}

bool cnt_client_on_host_mapping_exists(cnt_client_on_host_mapping *mapping, uint32_t index)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(index);

	cnt_client_on_host_to_dense_index *slot = &mapping->sparse[index];
	const uint32_t control_bit_mask = mapping->control_bit_mask;
	if ((slot->index & control_bit_mask) == control_bit_mask)
	{
		return false;
	}

	return true;
}

bool cnt_client_on_host_mapping_try_get(cnt_client_on_host_mapping *mapping, uint32_t index, cnt_client_on_host_index *out)
{
	CNT_NULL_CHECK(mapping);
	CNT_NULL_CHECK(out);

	if (!cnt_client_on_host_mapping_exists(mapping, index))
	{
		out = nullptr;
		return false;
	}

	cnt_client_on_host_to_dense_index *slot = &mapping->sparse[index];
	cnt_client_on_host_index *dense = &mapping->dense[slot->index];
	out->index = dense->index;

	return true;
}

bool cnt_client_on_host_mapping_create(cnt_client_on_host_mapping *mapping, cnt_client_on_host_index *out)
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
		uint32_t free_sparse_index = mapping->free_head;

		// Value stored in sparse[candidate] is invalid
		// We get the value, make it valid and assign it to free_head
		// By convenience, the free list head is always valid to use for indexing
		cnt_client_on_host_to_dense_index *sparse_index = &mapping->sparse[free_sparse_index];

		cnt_client_on_host_mapping_make_valid(mapping, sparse_index);
		mapping->free_head = sparse_index->index;

		sparse_index->index = next_dense;

		cnt_client_on_host_index *dense_index = &mapping->dense[next_dense];
		dense_index->index = free_sparse_index;
	}
	else
	{
		// Create a new one. We can safely use the dense list count
		// If no free list item is available, it implies the sparse lista and dense list
		// have the same length! In other words, everything is used nicely
		cnt_client_on_host_to_dense_index *sparse_index = &mapping->sparse[next_dense];
		sparse_index->index = next_dense;

		cnt_client_on_host_index *dense_index = &mapping->dense[next_dense];
		dense_index->index = next_dense;
	}
	cnt_client_on_host_index *dense = &mapping->dense[next_dense];
	out->index = dense->index;

	mapping->count = mapping->count + 1;
	return true;
}

bool cnt_client_on_host_mapping_remove(cnt_client_on_host_mapping *mapping, uint32_t index)
{
	CNT_NULL_CHECK(mapping);

	cnt_client_on_host_to_dense_index *slot = &mapping->sparse[index];
	if (!cnt_client_on_host_mapping_exists(mapping, slot->index))
	{
		return false;
	}

	// Update mapping
	{
		uint32_t last_dense = mapping->dense->index - 1;
		cnt_client_on_host_index *last_dense_index = &mapping->dense[last_dense];

		// Last sparse is now pointing to the removed index
		cnt_client_on_host_to_dense_index *last_sparse_index = &mapping->sparse[last_dense_index->index];
		last_sparse_index->index = slot->index;

		// Last dense index is getting copied over to dense data of the removed index
		cnt_client_on_host_index *dense_index = &mapping->dense[last_sparse_index->index];
		dense_index->index = last_dense_index->index;

		// Pop last element - We did a remove swap while maintaining the sparse-dense mapping
		mapping->count = last_dense;
	}

	// Update free list - Add the removed index (slot) to the free list by invalidating it
	// The index itself becomes the new head of the free list
	{
		slot->index = mapping->free_head;
		cnt_client_on_host_mapping_make_invalid(mapping, slot);
		mapping->free_head = index;
	}
	return true;
}

// Windows
#ifdef _WIN32
#include <ws2tcpip.h>
struct cnt_sock_internal
{
	SOCKET handle;
};
struct cnt_addr_internal
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
struct cnt_addr_internal // Not tested on Linux, only MacOS
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

#include "shared/fck_checks.h"

#include "lz4.h"

#define CNT_FOR(type, var_name, count) for (type var_name = 0; (var_name) < (count); (var_name)++)
#define CNT_SIZEOF_SMALLER(a, b) sizeof(a) < sizeof(b) ? sizeof(a) : sizeof(b)

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

static cnt_addr *copy_to_user_address(cnt_addr_internal *addr, cnt_addr *dst)
{
	SDL_zerop(dst);
	SDL_memcpy(dst, addr, CNT_SIZEOF_SMALLER(*dst, *addr));

	return dst;
}

static cnt_addr_internal *copy_to_internal_address(cnt_addr *addr, cnt_addr_internal *dst)
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
	return fcntl(sock->handle, F_SETFL, fcntl(handle, F_GETFL, 0) | O_NONBLOCK);
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

static bool cnt_addr_equals(cnt_addr *a, cnt_addr *b)
{
	if (a == nullptr || b == nullptr)
	{
		return false;
	}

	cnt_addr_internal lhs;
	copy_to_internal_address(a, &lhs);
	cnt_addr_internal rhs;
	copy_to_internal_address(b, &rhs);

	if (lhs.addrlen != rhs.addrlen)
	{
		return false;
	}

	return SDL_memcmp(&lhs.addr, &rhs.addr, lhs.addrlen) == 0;
}

cnt_addr *cnt_addr_create(cnt_addr *address, const char *ip, uint16_t port)
{
	SDL_assert(address && "Address is null pointer");

	// We only accept
	uint8_t buffer[128];
	int result = inet_pton(AF_INET, ip, &buffer);
	CHECK_ERROR(result == 1, "Cannot convert address from text to binary - invalid arguments", return {});

	cnt_addr_internal result_address;
	SDL_zero(result_address);

	result_address.in.sin_family = AF_INET;
	result_address.in.sin_addr = *(in_addr *)buffer;
	result_address.in.sin_port = port;
	result_address.addrlen = sizeof(sockaddr_in);

	copy_to_user_address(&result_address, address);

	return address;
}

static cnt_addr_container *cnt_addr_container_open(cnt_addr_container *container, uint32_t capacity)
{
	SDL_assert(container && "Container is null pointer");

	container->addresses = (cnt_addr *)SDL_malloc(sizeof(*container->addresses) * capacity);
	container->capacity = capacity;
	container->count = 0;
}

static cnt_addr *cnt_addr_container_add(cnt_addr_container *container, cnt_addr *address)
{
	// Add new address if not found!
	uint32_t at = container->count;
	SDL_memcpy(container->addresses + at, address, sizeof(*address));
	container->count = at + 1;
	return container->addresses + at;
}

static void cnt_addr_container_remove_swap_at(cnt_addr_container *container, uint32_t index)
{
	uint32_t last = container->count - 1;

	SDL_memcpy(container->addresses + index, container->addresses + last, sizeof(*container->addresses));
	container->count = last;
}

static cnt_addr_container *cnt_addr_container_close(cnt_addr_container *container)
{
	SDL_assert(container && "Container is null pointer");

	SDL_free(container->addresses);
	SDL_zerop(container);
}

cnt_sock *cnt_sock_open(cnt_sock *sock, const char *ip, uint16_t port)
{
	SDL_assert(sock && "Sock is null pointer");

	cnt_addr address;
	if (cnt_addr_create(&address, ip, port) == false)
	{
		return nullptr;
	}

	cnt_addr_internal addr_internal;
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

cnt_stream *cnt_stream_create(cnt_stream *stream, uint8_t *data, int capacity)
{
	SDL_assert(stream && "Stream is null pointer");

	stream->data = data;
	stream->capacity = capacity;
	stream->at = 0;
	return stream;
}

cnt_stream *cnt_stream_open(cnt_stream *stream, int capacity)
{
	SDL_assert(stream && "Stream is null pointer");

	stream->data = (uint8_t *)SDL_malloc(capacity);
	stream->capacity = capacity;
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

static void cnt_stream_read_string(cnt_stream *serialiser, uint8_t *value, uint16_t count)
{
	uint8_t *at = serialiser->data + serialiser->at;

	SDL_memcpy(value, at, count);

	serialiser->at = serialiser->at + (sizeof(*value) * count);
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

static void cnt_stream_write_string(cnt_stream *serialiser, uint8_t *value, uint16_t count)
{
	uint8_t *at = serialiser->data + serialiser->at;

	SDL_memcpy(at, value, count);

	serialiser->at = serialiser->at + (sizeof(*value) * count);
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

// Return type shall be typed and safe to use!
static cnt_stream cnt_stream_reserve(cnt_stream *serialiser, uint16_t count)
{
	int target = serialiser->at;
	uint8_t *at = serialiser->data + target;

	serialiser->at = serialiser->at + count;

	return {0, serialiser->capacity - target, at};
}

cnt_transport *cnt_transport_open(cnt_transport *transport, int capacity)
{
	SDL_assert(transport && "Transport is null pointer");

	cnt_stream_open(&transport->stream, capacity);
	return transport;
}

cnt_transport *cnt_transport_write(cnt_transport *transport, cnt_stream *stream)
{
	SDL_assert(transport && "Transport is null pointer");
	SDL_assert(stream && "Stream is null pointer");

	return transport;
}

cnt_transport *cnt_transport_read(cnt_transport *transport, cnt_stream *stream)
{
	SDL_assert(transport && "Transport is null pointer");
	SDL_assert(stream && "Stream is null pointer");

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

	connection->error = 0; // Should we have a special error code to indicate there is no address?
	return connection;
}

cnt_connection *cnt_connection_open(cnt_connection *connection, cnt_sock *src, cnt_addr *dst)
{
	SDL_assert(connection && "Connection is null");
	SDL_assert(src && "Socket (src) is null pointer");
	SDL_assert(dst && "Address (dst) is null pointer");

	SDL_memcpy(&connection->src, src, sizeof(*src));
	SDL_memcpy(&connection->dst, dst, sizeof(*dst));
	connection->error = 0;
	return connection;
}

cnt_connection *cnt_connection_send(cnt_connection *connection, cnt_stream *stream)
{
	SDL_assert(connection && "Connection is null pointer");
	SDL_assert(stream && "Stream is null pointer");

	cnt_sock_internal sock;
	copy_to_internal_sock(&connection->src, &sock);
	cnt_addr_internal addr;
	copy_to_internal_address(&connection->dst, &addr);

	int planned = stream->at;
	int result = sendto(sock.handle, (char *)stream->data, stream->at, 0, &addr.addr, addr.addrlen);

	if (result == -1)
	{
		connection->error = result;
		stream->at = planned;
		const int err = last_socket_error();
		if (would_block(err))
		{
			return connection;
		}
	}
	stream->at = 0;
	connection->error = 0;
	return connection;
}

static void query_address(cnt_sockaddr_storage *socket_address_storage, cnt_addr *out_address)
{
	cnt_addr_internal address = *(cnt_addr_internal *)socket_address_storage;
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

	int result = recvfrom(sock_internal.handle, (char *)stream->data, stream->capacity, 0, (struct sockaddr *)&storage, &socket_length);

	if (result == -1)
	{
		stream->at = 0;
		connection->error = result;

		const int err = last_socket_error();
		if (would_block(err))
		{
			stream->at = 0;
		}
		return nullptr;
	}
	else if (result > 0)
	{
		stream->at = result;
		connection->error = 0;

		query_address(&storage, &connection->dst);
	}

	return connection;
}

void cnt_connection_close(cnt_connection *connection)
{
	SDL_assert(connection && "Connection is null pointer");

	SDL_zerop(connection);
}

static cnt_addr *cnt_star_find(cnt_star *star, cnt_addr *address)
{
	CNT_FOR(uint32_t, index, star->destinations.count)
	{
		cnt_addr *destination = star->destinations.addresses + index;
		if (cnt_addr_equals(destination, address))
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

	cnt_addr_container_open(&star->destinations, max_connections);

	return star;
}

cnt_star *cnt_star_send(cnt_star *star, cnt_stream *stream)
{
	SDL_assert(star && "Star is null pointer");
	SDL_assert(stream && "Stream is null pointer");

	CNT_FOR(uint32_t, index, star->destinations.count)
	{
		cnt_addr *destination = star->destinations.addresses + index;

		cnt_connection connection;
		if (cnt_connection_open(&connection, &star->sock, destination))
		{
			cnt_connection_send(&connection, stream);
		}

		cnt_connection_close(&connection);
	}

	return star;
}

cnt_star *cnt_star_recv(cnt_star *star, cnt_addr *address, cnt_stream *stream)
{
	SDL_assert(star && "Star is null pointer");
	SDL_assert(address && "Address is null pointer");
	SDL_assert(stream && "Stream is null pointer");

	cnt_connection connection;
	cnt_connection_from_socket(&connection, &star->sock);

	cnt_stream sub_stream;
	cnt_stream_create(&sub_stream, stream->data, stream->capacity);

	if (cnt_connection_recv(&connection, &sub_stream))
	{
		cnt_addr *addr = cnt_star_find(star, &connection.dst);
		if (addr == nullptr)
		{
			// Add new address if not found!
			cnt_addr_container_add(&star->destinations, &connection.dst);
		}

		SDL_memcpy(address, &connection.dst, sizeof(connection.dst));

		uint8_t *next_buffer = sub_stream.data + sub_stream.at;
		int remaining_capacity = sub_stream.capacity - sub_stream.at;

		cnt_stream_create(&sub_stream, next_buffer, remaining_capacity);
	}
	return star;
}

cnt_star *cnt_star_add(cnt_star *star, cnt_addr *addr)
{
	SDL_assert(star && "Star is null pointer");
	SDL_assert(addr && "Addr is null pointer");

	SDL_assert(cnt_star_find(star, addr) != nullptr && "Addr is already added");

	cnt_addr_container_add(&star->destinations, addr);

	return star;
}

cnt_addr *cnt_star_remove(cnt_star *star, cnt_addr *addr)
{
	SDL_assert(star && "Star is null pointer");
	SDL_assert(addr && "Addr is null pointer");

	CNT_FOR(uint32_t, index, star->destinations.count)
	{
		cnt_addr *destination = star->destinations.addresses + index;
		if (cnt_addr_equals(destination, addr))
		{
			cnt_addr_container_remove_swap_at(&star->destinations, index);
			return addr;
		}
	}

	return nullptr;
}

void cnt_star_close(cnt_star *star)
{
	SDL_assert(star && "Star is null pointer");

	cnt_addr_container_close(&star->destinations);

	cnt_sock_close(&star->sock);

	SDL_zerop(star);
}

cnt_protocol_client *cnt_protocol_client_create(cnt_protocol_client *client_protocol, cnt_client *client)
{
	SDL_assert(client && "Client is null pointer");
	SDL_assert(client_protocol && "Protocol is null pointer");

	bool is_protocol_invalid = client->state == CNT_PROTOCOL_STATE_CLIENT_NONE;
	if (is_protocol_invalid)
	{
		return nullptr;
	}

	SDL_zerop(client_protocol);

	client_protocol->prefix = 8008;
	client_protocol->state = client->state;

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

cnt_protocol_host *cnt_protocol_host_apply(cnt_protocol_host *host_protocol, cnt_client *client)
{
	SDL_assert(client && "Client is null pointer");
	SDL_assert(host_protocol && "Protocol is null pointer");

	bool is_protocol_invalid = host_protocol->prefix != 8008 || host_protocol->state == CNT_PROTOCOL_STATE_HOST_NONE ||
	                           client->state == CNT_PROTOCOL_STATE_CLIENT_NONE;

	if (is_protocol_invalid)
	{
		return nullptr;
	}

	switch (host_protocol->state)
	{
	case CNT_PROTOCOL_STATE_HOST_CHALLENGE: {
		cnt_stream stream;
		cnt_stream_create(&stream, host_protocol->extra_payload, host_protocol->extra_payload_count);
		cnt_protocol_secret_read(&client->secret, &stream);
		if (client->state != CNT_SECRET_STATE_OUTDATED)
		{
			break;
		}
		client->secret.public_value = client->secret.public_value;

		client->state = CNT_PROTOCOL_STATE_CLIENT_ANSWER;
		break;
	}
	case CNT_PROTOCOL_STATE_HOST_RESOLUTION_REJECT:
		// Umm...
		return nullptr;
	case CNT_PROTOCOL_STATE_HOST_OK:
		client->state = CNT_PROTOCOL_STATE_CLIENT_OK;
		break;
	}
	return host_protocol;
}

cnt_protocol_host *cnt_protocol_host_create(cnt_protocol_host *host_protocol, cnt_host *host, cnt_client_state_on_host *client_state)
{
	SDL_assert(host && "Host is null pointer");
	SDL_assert(client_state && "State is null pointer");

	SDL_assert(host_protocol && "Protocol is null pointer");

	bool is_protocol_invalid = client_state->protocol == CNT_PROTOCOL_STATE_HOST_NONE;
	if (is_protocol_invalid)
	{
		return nullptr;
	}

	SDL_zerop(host_protocol);

	host_protocol->prefix = 8008;
	host_protocol->state = client_state->protocol;

	if (host_protocol->state == CNT_PROTOCOL_STATE_HOST_CHALLENGE)
	{
		cnt_stream stream;
		cnt_stream_create(&stream, host_protocol->extra_payload, sizeof(host_protocol->extra_payload));
		cnt_protocol_secret_write(&client_state->secret, &stream);
		host_protocol->extra_payload_count = stream.at;
	}

	return host_protocol;
}

cnt_protocol_client *cnt_protocol_client_apply(cnt_protocol_client *client_protocol, cnt_host *host, cnt_client_state_on_host *client_state)
{
	SDL_assert(host && "Host is null pointer");
	SDL_assert(client_state && "State is null pointer");
	SDL_assert(client_protocol && "Protocol is null pointer");

	bool is_protocol_invalid = client_protocol->prefix != 8008 || client_protocol->state == CNT_PROTOCOL_STATE_CLIENT_NONE ||
	                           client_state->protocol == CNT_PROTOCOL_STATE_HOST_NONE;

	if (is_protocol_invalid)
	{
		return nullptr;
	}

	cnt_stream stream;
	cnt_stream_create(&stream, client_protocol->extra_payload, client_protocol->extra_payload_count);
	cnt_protocol_secret_read(&client_state->secret, &stream);

	const uint16_t seed = 69;

	if (client_protocol->state == CNT_PROTOCOL_STATE_CLIENT_REQUEST)
	{
		client_state->secret.private_value = 420;
		client_state->secret.public_value = seed ^ client_state->secret.private_value;
		client_state->secret.state = CNT_SECRET_STATE_OUTDATED;
		client_state->protocol = CNT_PROTOCOL_STATE_HOST_CHALLENGE;

		// TODO: fix up assigning a client_id to the client which gets then sent out in the host_create function.
		cnt_client_on_host_index client_id;
		if (!cnt_client_on_host_mapping_create(&host->mapping, &client_id))
		{
			// Too full 
			return nullptr;
		}
		client_state->id.index = client_id.index;

		return client_protocol;
	}

	bool client_value = seed ^ client_state->secret.public_value;
	bool is_correct = client_state->secret.private_value == client_value;
	if (client_protocol->state == CNT_PROTOCOL_STATE_CLIENT_ANSWER)
	{
		client_state->secret.state = is_correct ? CNT_SECRET_STATE_ACCEPTED : CNT_SECRET_STATE_REJECTED;
		client_state->protocol = is_correct ? CNT_PROTOCOL_STATE_HOST_OK : CNT_PROTOCOL_STATE_HOST_RESOLUTION_REJECT;
		return client_protocol;
	}

	if (client_protocol->state == CNT_PROTOCOL_STATE_CLIENT_OK)
	{
		client_state->secret.state = is_correct ? CNT_SECRET_STATE_ACCEPTED : CNT_SECRET_STATE_REJECTED;
		return client_protocol;
	}
	return client_protocol;
}

cnt_host *cnt_host_open(cnt_host *host, cnt_sock *sock, uint32_t max_connections)
{
	SDL_assert(host && "Host is null pointer");
	SDL_assert(sock && "Sock is null pointer");

	cnt_star_open(&host->star, sock, max_connections);
	cnt_client_on_host_mapping_open(&host->mapping, max_connections);

	host->client_states = (cnt_client_state_on_host *)SDL_malloc(sizeof(*host->client_states) * max_connections);

	return host;
}

cnt_host *cnt_host_send(cnt_host *host, cnt_stream *stream)
{
	SDL_assert(host && "Host is null pointer");

	cnt_star_send(&host->star, stream);

	// HOW TO WRITE THIS LOGIC?! WE NEED TO GET AND SET TRAFFIC
	// update state of pending clients
	// broadcast stream to established clients

	return nullptr;
}

cnt_host *cnt_host_recv(cnt_host *host, cnt_stream *stream)
{
	SDL_assert(host && "Host is null pointer");

	// HOW TO WRITE THIS LOGIC?! WE NEED TO GET AND SET TRAFFIC
	// update state of pending clients
	// broadcast stream to established clients

	return nullptr;
}

cnt_host *cnt_host_update(cnt_host *host)
{
	SDL_assert(host && "Host is null pointer");

	cnt_client_on_host_mapping *mapping = &host->mapping;
	for (uint32_t index = 0; index < mapping->count; index++)
	{
		cnt_client_on_host_index dense_index = mapping->dense[index];
	}
	// HOW TO WRITE THIS LOGIC?! WE NEED TO GET AND SET TRAFFIC
	// update state of pending clients
	// broadcast stream to established clients

	return nullptr;
}

cnt_addr *cnt_host_kick(cnt_host *host, cnt_addr *addr)
{
	SDL_assert(host && "Host is null pointer");

	if (cnt_star_remove(&host->star, addr) == nullptr)
	{
		return nullptr;
	}

	return addr;
}

void cnt_host_close(cnt_host *host)
{
	SDL_assert(host && "Host is null pointer");

	// We can kick all players before hehe

	SDL_free(host->client_states);

	cnt_client_on_host_mapping_close(&host->mapping);

	cnt_star_close(&host->star);

	SDL_zerop(host);
}

cnt_client *cnt_client_open(cnt_client *client, cnt_connection *connection)
{
	SDL_assert(client && "Client is null pointer");
	SDL_assert(connection && "Connection is null pointer");

	SDL_zerop(client);

	cnt_connection_open(&client->connection, &connection->src, &connection->dst);

	client->state = CNT_PROTOCOL_STATE_CLIENT_REQUEST;

	return client;
}

cnt_protocol_client *cnt_client_update(cnt_client *client, cnt_protocol_client *protocol)
{
	SDL_assert(client && "Client is null pointer");
	SDL_assert(protocol && "Protocol is null pointer");

	return protocol;
}

void cnt_client_close(cnt_client *client)
{
	SDL_assert(client && "Client is null pointer");

	cnt_connection_close(&client->connection);
}

// Examples:
void example_sender()
{
	// TODO: Embed address directly into socket...
	// It does not make sense to separate these two concepts
	// when you HAVE to bind an address anyway
	// Startup
	cnt_addr host_address;
	cnt_addr_create(&host_address, "192.188.1.69", 42069);

	cnt_sock client_socket;
	cnt_sock_open(&client_socket, "0.0.0.0", 69420);

	cnt_connection connection;
	cnt_connection_open(&connection, &client_socket, &host_address);

	cnt_client client;
	cnt_client_open(&client, &connection);

	cnt_protocol_client client_protocol;
	// Add open/close util

	cnt_transport transport;
	cnt_transport_open(&transport, 1 << 16);

	cnt_compression compression;
	cnt_compression_open(&compression, 1 << 16);

	cnt_stream stream;
	cnt_stream_open(&stream, 1024);

	// Tick
	cnt_protocol_client *protocol = cnt_client_update(&client, &client_protocol);
	if (protocol != nullptr)
	{
		cnt_protocol_client_write(protocol, &transport.stream);
		if (protocol->state == CNT_PROTOCOL_OK)
		{
			// Put data on the client ;)
		}
	}

	cnt_compress(&compression, &transport.stream);

	cnt_connection_send(&connection, &compression.stream);

	// Cleanup
	cnt_stream_close(&stream);

	cnt_client_close(&client);

	cnt_compression_close(&compression);

	cnt_transport_close(&transport);

	cnt_connection_close(&connection);

	cnt_sock_close(&client_socket);
}

void example_listener()
{
	// Startup
	cnt_sock server_socket;
	cnt_sock_open(&server_socket, "192.188.1.69", 42069);

	cnt_star star;
	cnt_star_open(&star, &server_socket, 32);

	cnt_transport transport;
	cnt_transport_open(&transport, 1 << 16);

	cnt_compression compression;
	cnt_compression_open(&compression, 1 << 16);

	// Tick
	cnt_addr recv_addr;
	SDL_zero(recv_addr);
	while (cnt_star_recv(&star, &recv_addr, &compression.stream))
	{
		cnt_decompress(&compression, &transport.stream);
	}

	// Cleanup
	cnt_compression_close(&compression);

	cnt_transport_close(&transport);

	cnt_star_close(&star);
}