#ifndef CNT_PEERS_INCLUDED
#define CNT_PEERS_INCLUDED

#include "ecs/fck_sparse_list.h"
#include "net/cnt_net_types.h"
#include "net/cnt_peer.h"
#include <SDL3/SDL_assert.h>
struct cnt_peers
{
	constexpr static cnt_peer_id max_peers = 32;
	constexpr static cnt_peer_id invalid_peer = ~0;

	cnt_peer_id self;
	cnt_peer_id host;

	fck_sparse_list<cnt_peer_id, cnt_peer> peers;

	fck_sparse_lookup<cnt_connection_id, cnt_peer_id> connection_to_peer;
};

void cnt_peers_alloc(cnt_peers *peers);
void cnt_peers_free(cnt_peers *peers);
void cnt_peers_clear(cnt_peers *peers);

void cnt_peers_set_self(cnt_peers *peers, cnt_peer_id id);
void cnt_peers_set_host(cnt_peers *peers, cnt_peer_id id);
cnt_peer *cnt_peers_view_peer_from_index(cnt_peers *peers, cnt_peer_id index);
bool cnt_peers_is_hosting(cnt_peers *peers);
bool cnt_peers_is_ok(cnt_peers *peers);

bool cnt_peers_try_add(cnt_peers *peers, cnt_connection_handle const *connection_handle, cnt_peer **out_peer);
bool cnt_peers_try_get_peer_from_connection(cnt_peers *peers, cnt_connection_handle const *connection, cnt_peer *out_peer);
void cnt_peers_emplace(cnt_peers *peers, cnt_peer *peer, cnt_connection_handle const *connection_handle);

#endif // !CNT_PEERS_INCLUDED