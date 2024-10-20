#include "net/cnt_transport.h"

#include <SDL3/SDL_stdinc.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "SDL3/SDL_assert.h"
#include "fck_checks.h"

typedef socklen_t cnt_socklen;
typedef struct sockaddr_storage cnt_sockaddr_storage;

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

cnt_address *cnt_address_from_string(const char *text, uint16_t port)
{
	// TODO: make this prettier
	sockaddr_in6 in6_address;

	addrinfo hint;
	SDL_zero(hint);
	hint.ai_socktype = SOCK_DGRAM;
	hint.ai_family = AF_INET;

	sockaddr_in in_address;
	SDL_zero(in_address);
	hint.ai_addr = (sockaddr *)&in_address;
	hint.ai_addrlen = sizeof(in_address);

	// AF_INET6 does not recognize IPv4 addresses. An explicit
	// IPv4-mapped IPv6 address must be supplied in src instead.
	int result = inet_pton(AF_INET, text, &in_address);
	CHECK_ERROR(result != -1, "Cannot convert address from text to binary - invalid arguments", return nullptr);

	if (result == 0)
	{
		hint.ai_family = AF_INET6;
		SDL_zero(in6_address);
		hint.ai_addr = (sockaddr *)&in6_address;
		hint.ai_addrlen = sizeof(in6_address);

		result = inet_pton(AF_INET, text, &in6_address);
		CHECK_ERROR(result != -1, "Cannot convert address from text to binary - invalid arguments", return nullptr);
		CHECK_ERROR(result != 0, "Cannot convert address from text to binary - Not supported address family", return nullptr);
	}
	// hint.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_PASSIVE;

	char service[16];
	SDL_snprintf(service, sizeof(service), "%d", (int)port);

	addrinfo *addr = nullptr;
	int rc = getaddrinfo(nullptr, service, &hint, &addr);
	CHECK_CRITICAL(rc == 0, create_get_address_error_info_string(rc), return nullptr);
	return addr;
}

cnt_address *get_address(cnt_sockaddr_storage *socket_address_storage)
{
	addrinfo hint;
	SDL_zero(hint);
	hint.ai_socktype = SOCK_DGRAM;
	hint.ai_family = socket_address_storage->ss_family;
	hint.ai_addr = (sockaddr *)socket_address_storage;
	hint.ai_addrlen = socket_address_storage->ss_len;
	// hint.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_PASSIVE ;

	sockaddr_in *fakeipv4 = (sockaddr_in *)socket_address_storage;
	uint16_t port = ntohs(fakeipv4->sin_port);

	char service[16];
	SDL_snprintf(service, sizeof(service), "%d", (int)port);

	addrinfo *addr = nullptr;
	int rc = getaddrinfo(nullptr, service, &hint, &addr);
	CHECK_CRITICAL(rc == 0, create_get_address_error_info_string(rc), return nullptr);
	return addr;
}

cnt_address *make_any_address_with_port(uint16_t port)
{
	addrinfo hints;
	SDL_zero(hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_PASSIVE;

	char service[16];
	SDL_snprintf(service, sizeof(service), "%d", (int)port);

	addrinfo *addrwithport = NULL;
	int rc = getaddrinfo(nullptr, service, &hints, &addrwithport);
	CHECK_CRITICAL(rc == 0, create_get_address_error_info_string(rc), return nullptr);

	return addrwithport;
}

void cnt_address_free(cnt_address *address)
{
	freeaddrinfo(address);
}

cnt_socket cnt_socket_create(const char *ip, uint16_t port)
{
	struct addrinfo *address = cnt_address_from_string(ip, port);
	if (!address)
	{
		return -1;
	}

	cnt_socket socket_handle = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
	CHECK_CRITICAL(socket_handle != -1, "Failed to create socket", freeaddrinfo(address); return -1);

	const bool non_block_sucess = make_socket_non_blocking(socket_handle) >= 0;
	CHECK_CRITICAL(non_block_sucess, "Failed to make socket non-blocking", close_socket(socket_handle); freeaddrinfo(address); return -1);

	const int binding_failed = bind(socket_handle, address->ai_addr, address->ai_addrlen);
	freeaddrinfo(address);

	CHECK_CRITICAL(binding_failed != -1, "Failed to bint socket", close_socket(socket_handle); return -1);
	return socket_handle;
}

size_t cnt_send(cnt_socket socket, void *buf, size_t count, cnt_address *to)
{
	SDL_assert(socket != -1);
	ssize_t bytes_sent = sendto(socket, buf, count, 0, to->ai_addr, to->ai_addrlen);

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

size_t cnt_recv(cnt_socket socket, uint8_t *buf, size_t count, cnt_address **from)
{
	SDL_assert(socket != -1);
	cnt_sockaddr_storage storage;

	cnt_socklen socket_length = sizeof(storage);
	ssize_t bytes_rcvd = recvfrom(socket, buf, count, 0, (struct sockaddr *)&storage, &socket_length);

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

bool cnt_address_equals(cnt_address const *lhs, cnt_address const *rhs)
{
	if (lhs == nullptr || rhs == nullptr)
	{
		return false;
	}
	if (lhs->ai_family != rhs->ai_family)
	{
		return false;
	}
	if (lhs->ai_protocol != rhs->ai_protocol)
	{
		return false;
	}
	if (lhs->ai_addrlen != rhs->ai_addrlen)
	{
		return false;
	}

	return SDL_memcmp(lhs->ai_addr, rhs->ai_addr, lhs->ai_addrlen) == 0;
}

void cnt_address_as_string(cnt_address *address, char *buffer, size_t length)
{
	if (address->ai_family == AF_INET6)
	{
		SDL_snprintf(buffer, length, "TODO: Print IPv6");
	}

	if (address->ai_family == AF_INET)
	{
		sockaddr_in *in_addr = (sockaddr_in *)address->ai_addr;
		uint8_t *ptr = (uint8_t *)&in_addr->sin_addr;
		SDL_snprintf(buffer, length, "%d.%d.%d.%d:%d", ptr[0], ptr[1], ptr[2], ptr[3], ntohs(in_addr->sin_port));
	}
}