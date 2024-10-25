#ifndef CNT_TRANSPORT_INCLUDED
#define CNT_TRANSPORT_INCLUDED

#include <SDL3/SDL_stdinc.h>

typedef int cnt_socket;

struct cnt_address
{
	uint8_t data[64];
};

constexpr const char *CNT_IPV4_BROADCAST = "255.255.255.255";
constexpr const char *CNT_IPV4_ANY = "0.0.0.0";

cnt_socket cnt_socket_create(const char *ip, uint16_t port);

size_t cnt_send(cnt_socket socket, void *buf, size_t count, cnt_address *to);

size_t cnt_recv(cnt_socket socket, uint8_t *buf, size_t count, cnt_address *from);

void cnt_socket_destroy(cnt_socket socket);

cnt_address cnt_address_from_string(const char *ip, uint16_t port);

bool cnt_address_equals(cnt_address const *lhs, cnt_address const *rhs);

void cnt_address_as_string(cnt_address *address, char *buffer, size_t length);

#endif // CNT_TRANSPORT_INCLUDED