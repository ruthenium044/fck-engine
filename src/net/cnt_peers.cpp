#include "net/cnt_peers.h"
#include "net/cnt_session.h"

#include <SDL3/SDL_log.h>

void cnt_peers_alloc(cnt_peers *peers)
{
	SDL_assert(peers != nullptr);
	SDL_zerop(peers);

	peers->self = cnt_peers::invalid_peer;
	peers->host = cnt_peers::invalid_peer;

	fck_sparse_list_alloc(&peers->peers, cnt_peers::max_peers);
	fck_sparse_lookup_alloc(&peers->connection_to_peer, 64, cnt_peers::invalid_peer);
}

void cnt_peers_clear(cnt_peers *peers)
{
	SDL_assert(peers != nullptr);

	peers->self = cnt_peers::invalid_peer;
	peers->host = cnt_peers::invalid_peer;

	fck_sparse_list_clear(&peers->peers);
	fck_sparse_lookup_clear(&peers->connection_to_peer);
}

void cnt_peers_free(cnt_peers *peers)
{
	SDL_assert(peers != nullptr);

	peers->self = cnt_peers::invalid_peer;
	peers->host = cnt_peers::invalid_peer;

	fck_sparse_list_free(&peers->peers);
	fck_sparse_lookup_free(&peers->connection_to_peer);
}

void cnt_peers_set_self(cnt_peers *peers, cnt_peer_id id)
{
	SDL_assert(peers != nullptr);

	peers->self = id;
}

void cnt_peers_set_host(cnt_peers *peers, cnt_peer_id id)
{
	SDL_assert(peers != nullptr);

	peers->host = id;
}

cnt_peer *cnt_peers_view_peer_from_index(cnt_peers *peers, cnt_peer_id index)
{
	SDL_assert(peers != nullptr);

	return fck_sparse_list_view(&peers->peers, index);
}

bool cnt_peers_is_hosting(cnt_peers *peers)
{
	SDL_assert(peers != nullptr);

	return peers->self == peers->host && peers->self != cnt_peers::invalid_peer;
}

bool cnt_peers_is_ok(cnt_peers *peers)
{
	SDL_assert(peers != nullptr);

	if (peers->self == cnt_peers::invalid_peer)
	{
		return false;
	}

	cnt_peer *peer = cnt_peers_view_peer_from_index(peers, peers->self);
	if (peer != nullptr)
	{
		return peer->state == cnt_peer_STATE_OK;
	}
	return false;
}

bool cnt_peers_try_add(cnt_peers *peers, cnt_connection_handle const *connection_handle, cnt_peer **out_peer)
{
	SDL_assert(peers != nullptr);

	cnt_peer peer;
	SDL_zero(peer);

	peer.state = cnt_peer_STATE_NONE;
	peer.peer_id = fck_sparse_list_add(&peers->peers, &peer);
	if (!fck_sparse_list_exists(&peers->peers, peer.peer_id))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot add peer - No space left. TODO: Implement disconnect functionality");
		return false;
	}

	if (connection_handle != nullptr)
	{
		fck_sparse_lookup_set(&peers->connection_to_peer, connection_handle->id, &peer.peer_id);
	}
	// When we succeed we can pass it into the view
	cnt_peer *private_peer = fck_sparse_list_view(&peers->peers, peer.peer_id);
	*private_peer = peer;
	*out_peer = private_peer;
	return true;
}

bool cnt_peers_try_get_peer_from_connection(cnt_peers *peers, cnt_connection_handle const *connection, cnt_peer *out_peer)
{
	SDL_assert(peers != nullptr);
	SDL_assert(out_peer != nullptr);
	if (connection == nullptr)
	{
		return false;
	}
	cnt_peer_id peer_id;
	bool exists = fck_sparse_lookup_try_get(&peers->connection_to_peer, connection->id, &peer_id);
	if (exists && peer_id != cnt_peers::invalid_peer)
	{
		*out_peer = *fck_sparse_list_view(&peers->peers, peer_id);
		return true;
	}

	return false;
}

void cnt_peers_emplace(cnt_peers *peers, cnt_peer *peer, cnt_connection_handle const *connection_handle)
{
	SDL_assert(peers != nullptr);
	SDL_assert(peer != nullptr);

	if (connection_handle != nullptr)
	{
		fck_sparse_lookup_set(&peers->connection_to_peer, connection_handle->id, &peer->peer_id);
	}
	fck_sparse_list_emplace(&peers->peers, peer->peer_id, peer);
}