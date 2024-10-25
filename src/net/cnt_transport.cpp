#include "net/cnt_transport.h"

#include <errno.h>
#include <SDL3/SDL_stdinc.h>
#include <string.h>

// Windows
#ifdef _WIN32
#include <ws2tcpip.h>
struct cnt_address_internal
{
	union {
		ADDRESS_FAMILY family; // Address family.
		sockaddr addr;         // base
		sockaddr_in in;        // ipv4
		sockaddr_in6 in6;      // ipv6
	};
	uint8_t addrlen;
};
#else // MacOS (Maybe Linux, idk)
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
struct cnt_address_internal // Not tested on Linux, only MacOS
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

#include "SDL3/SDL_assert.h"
#include "fck_checks.h"

typedef socklen_t cnt_socklen;
typedef struct sockaddr_storage cnt_sockaddr_storage;

static_assert(sizeof(cnt_address) >= sizeof(cnt_address_internal),
              "User address type must be large enough than the internal address type!");

static cnt_address *as_user_address(cnt_address_internal *addr)
{
	return (cnt_address *)addr;
}

static cnt_address_internal *as_internal_address(cnt_address *addr)
{
	return (cnt_address_internal *)addr;
}

static cnt_address const *as_user_address(cnt_address_internal const *addr)
{
	return (cnt_address *)addr;
}

static cnt_address_internal const *as_internal_address(cnt_address const *addr)
{
	return (cnt_address_internal *)addr;
}

static int close_socket(cnt_socket handle)
{
#ifdef _WIN32
	return closesocket(handle);
#else
	return close(handle);
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

static int make_socket_non_blocking(cnt_socket handle)
{
#ifdef _WIN32
	DWORD one = 1;
	return ioctlsocket(handle, FIONBIO, &one);
#else
	return fcntl(handle, F_SETFL, fcntl(handle, F_GETFL, 0) | O_NONBLOCK);
#endif
}

static SDL_bool would_block(const int err)
{
#ifdef _WIN32
	return (err == WSAEWOULDBLOCK) ? SDL_TRUE : SDL_FALSE;
#else
	return ((err == EWOULDBLOCK) || (err == EAGAIN) || (err == EINPROGRESS)) ? SDL_TRUE : SDL_FALSE;
#endif
}

cnt_address cnt_address_from_string(const char *text, uint16_t port)
{
	// AF_INET6 does not recognize IPv4 addresses. An explicit
	// IPv4-mapped IPv6 address must be supplied in src instead.
	uint8_t buffer[128];
	int result = inet_pton(AF_INET, text, &buffer);
	CHECK_ERROR(result != -1, "Cannot convert address from text to binary - invalid arguments", return {});
	if (result == 1)
	{
		cnt_address_internal result_address;
		SDL_zero(result_address);

		result_address.in.sin_family = AF_INET;
		result_address.in.sin_addr = *(in_addr *)&buffer;
		result_address.in.sin_port = port;
		result_address.addrlen = sizeof(sockaddr_in);
		return *as_user_address(&result_address);
	}
	result = inet_pton(AF_INET6, text, &buffer);
	CHECK_ERROR(result != -1, "Cannot convert address from text to binary - invalid arguments", return {});
	CHECK_ERROR(result != 0, "Cannot convert address from text to binary - Not supported address family", return {});

	cnt_address_internal result_address;
	SDL_zero(result_address);

	result_address.in6.sin6_addr = *(in6_addr *)&buffer;
	result_address.in6.sin6_family = AF_INET6;
	result_address.in6.sin6_port = port;
	result_address.addrlen = sizeof(sockaddr_in6);

	return *as_user_address(&result_address);
}

cnt_address get_address(cnt_sockaddr_storage *socket_address_storage)
{
	cnt_address_internal address = *(cnt_address_internal *)socket_address_storage;
	switch (address.family)
	{
	case AF_INET:
		address.addrlen = sizeof(sockaddr_in);
		break;
	case AF_INET6:
		address.addrlen = sizeof(sockaddr_in6);
		break;
	default:
		return {/* Empty address */};
	}

	return *(cnt_address *)&address;
}

cnt_socket cnt_socket_create(const char *ip, uint16_t port)
{
	cnt_address user_address = cnt_address_from_string(ip, port);
	cnt_address_internal *address = as_internal_address(&user_address);
	if (address->family == 0)
	{
		return -1;
	}

	cnt_socket socket_handle = socket(address->family, SOCK_DGRAM, 0);
	CHECK_CRITICAL(socket_handle != -1, "Failed to create socket", return -1);

	const bool non_block_sucess = make_socket_non_blocking(socket_handle) >= 0;
	CHECK_CRITICAL(non_block_sucess, "Failed to make socket non-blocking", close_socket(socket_handle); return -1);

	const int binding_failed = bind(socket_handle, &address->addr, address->addrlen);

	CHECK_CRITICAL(binding_failed != -1, "Failed to bint socket", close_socket(socket_handle); return -1);
	return socket_handle;
}

size_t cnt_send(cnt_socket socket, void *buf, size_t count, cnt_address *to)
{
	SDL_assert(socket != -1);
	cnt_address_internal *address = as_internal_address(to);
	size_t bytes_sent = sendto(socket, (char *)buf, count, 0, &address->addr, address->addrlen);

	if (bytes_sent == -1)
	{
		const int err = last_socket_error();
		if (would_block(err))
		{
			return 0;
		}
	}
	CHECK_ERROR(bytes_sent != -1, "Failed to send bytes");
	return bytes_sent;
}

size_t cnt_recv(cnt_socket socket, uint8_t *buf, size_t count, cnt_address *from)
{
	SDL_assert(socket != -1);
	cnt_sockaddr_storage storage;

	cnt_socklen socket_length = sizeof(storage);
	size_t bytes_rcvd = recvfrom(socket, (char *)buf, count, 0, (struct sockaddr *)&storage, &socket_length);

	if (bytes_rcvd == -1)
	{
		const int err = last_socket_error();
		if (would_block(err))
		{
			return 0;
		}
	}

	if (bytes_rcvd > 0)
	{
		*from = get_address(&storage);
	}

	CHECK_ERROR(bytes_rcvd != -1, "Failed to recv bytes");
	return bytes_rcvd;
}

void cnt_socket_destroy(cnt_socket socket)
{
	SDL_assert(socket != -1);
	close_socket(socket);
}

bool cnt_address_equals(cnt_address const *a, cnt_address const *b)
{
	cnt_address_internal const *lhs = as_internal_address(a);
	cnt_address_internal const *rhs = as_internal_address(b);
	if (lhs == nullptr || rhs == nullptr)
	{
		return false;
	}
	if (lhs->addrlen != rhs->addrlen)
	{
		return false;
	}

	return SDL_memcmp(&lhs->addr, &rhs->addr, lhs->addrlen) == 0;
}

void cnt_address_as_string(cnt_address *addr, char *buffer, size_t length)
{
	cnt_address_internal *address = as_internal_address(addr);

	if (address->family == AF_INET6)
	{
		SDL_snprintf(buffer, length, "TODO: Print IPv6");
	}

	if (address->family == AF_INET)
	{
		sockaddr_in *in_addr = &address->in;
		uint8_t *ptr = (uint8_t *)&in_addr->sin_addr;
		SDL_snprintf(buffer, length, "%d.%d.%d.%d:%d", ptr[0], ptr[1], ptr[2], ptr[3], ntohs(in_addr->sin_port));
	}
}