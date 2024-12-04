#include "net/cnt_core.h"

#include "core/fck_time.h"

// TODO: Fix up unordered UDP packets problem! :)
// TODO: correct Quake delta compression and snapshot interpolation, yaay

void cnt_networking_process_recv_message_unicast(fck_ecs *ecs, fck_serialiser *serialiser, cnt_connection_handle const *connection)
{
	// callback required
	cnt_session_callbacks *callbacks = fck_ecs_unique_view<cnt_session_callbacks>(ecs);

	if (callbacks == nullptr || callbacks->on_message == nullptr)
	{
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "No callback set to process messages");
		return;
	}

	cnt_on_message_params data;
	data.ecs = ecs;
	data.serialiser = serialiser;
	data.connection = connection;
	callbacks->on_message(&data);
}

void cnt_networking_process_recv_replication_broadcast(fck_ecs *ecs, fck_serialiser *serialiser, cnt_connection_handle const *connection)
{
	cnt_peers *peers = fck_ecs_unique_view<cnt_peers>(ecs);
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);

	// Peer info
	cnt_networking_segment_type peers_segment;
	fck_serialise(serialiser, &peers_segment);
	SDL_assert(peers_segment == CNT_NETWORK_SEGMENT_TYPE_PEERS);

	cnt_peer_id peer_count = 0;
	fck_serialise(serialiser, &peer_count);
	for (cnt_peer_id index = 0; index < peer_count; index++)
	{
		cnt_peer avatar = {cnt_peers::invalid_peer};
		fck_serialise(serialiser, &avatar);
		SDL_assert(avatar.peer_id != cnt_peers::invalid_peer);

		cnt_peer *peer = cnt_peers_view_peer_from_index(peers, index);
		if (peer != nullptr)
		{
			// We can only BUILD up peer state, never go back.
			// If a peer reached a specific state, it can only remain in there
			avatar.state = SDL_max(avatar.state, peer->state);
		}
		cnt_peers_emplace(peers, &avatar, nullptr);
	}

	cnt_peer peer;
	if (cnt_peers_try_get_peer_from_connection(peers, connection, &peer))
	{
		if (peer.state == CNT_PEER_STATE_OK)
		{
			// ECS snapshot
			cnt_networking_segment_type ecs_segment;
			fck_serialise(serialiser, &ecs_segment);
			SDL_assert(ecs_segment == CNT_NETWORK_SEGMENT_TYPE_ECS);

			if (cnt_peers_is_hosting(peers))
			{
				// Receive from client - Partial is ok
				fck_ecs_snapshot_load_partial(ecs, serialiser);
			}
			else
			{
				// Receive from host - Always FULL state
				// TODO: Implement a backbuffer ECS so we have a reliable storage
				// TODO: Using the serialiser for a back-copy is a bit hardcore
				fck_serialiser temp;
				fck_serialiser_alloc(&temp);
				fck_serialiser_byte_writer(&temp.self);

				// Copy all entity data that have the type fck_authority into a buffer
				fck_ecs::sparse_array<fck_authority> *entities = fck_ecs_view_single<fck_authority>(ecs);
				fck_ecs_snapshot_store_partial(ecs, &temp, &entities->owner);

				fck_ecs_snapshot_load(ecs, serialiser);
				fck_serialiser_reset(&temp);

				fck_serialiser_byte_reader(&temp.self);
				// Re-apply previously stored buffer back on the ECS
				fck_ecs_snapshot_load_partial(ecs, &temp);

				fck_serialiser_free(&temp);
			}
		}
	}
}

void cnt_networking_process_recv(fck_ecs *ecs)
{
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);

	cnt_memory_view memory_view;
	while (cnt_session_try_receive_from(session, &memory_view))
	{
		// Setup
		fck_serialiser serialiser;
		fck_serialiser_create(&serialiser, memory_view.data, memory_view.length);
		fck_serialiser_byte_reader(&serialiser.self);

		// Header
		cnt_networking_communication_mode mode;
		fck_serialise(&serialiser, &mode);

		// This shit can happen in ANY order
		// FUCK ME
		switch (mode)
		{
		case CNT_NETWORK_COMMUNICATION_MODE_MESSAGE_UNICAST:
			cnt_networking_process_recv_message_unicast(ecs, &serialiser, &memory_view.info.connection);
			break;
		case CNT_NETWORK_COMMUNICATION_MODE_REPLICATION_BROADCAST:
			cnt_networking_process_recv_replication_broadcast(ecs, &serialiser, &memory_view.info.connection);
			break;
		default:
			SDL_assert(false && "Great message buddy. The communication mode we received is unknown lol");
			break;
		}
	}
}

void cnt_networking_process_send(fck_ecs *ecs)
{
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);
	cnt_peers *peers = fck_ecs_unique_view<cnt_peers>(ecs);

	// Setup
	fck_serialiser serialiser;
	fck_serialiser_alloc(&serialiser);
	fck_serialiser_byte_writer(&serialiser.self);

	// Header
	cnt_networking_communication_mode mode = CNT_NETWORK_COMMUNICATION_MODE_REPLICATION_BROADCAST;
	fck_serialise(&serialiser, &mode);

	// Peer info
	cnt_networking_segment_type peers_segment = CNT_NETWORK_SEGMENT_TYPE_PEERS;
	fck_serialise(&serialiser, &peers_segment);

	cnt_peer_id peer_count = peers->peers.dense.count;
	fck_serialise(&serialiser, &peer_count);
	for (fck_item<cnt_peer_id, cnt_peer> item : &peers->peers)
	{
		SDL_assert(*item.index == item.value->peer_id);
		fck_serialise(&serialiser, item.value);
	}

	// ECS snapshot
	// Send out
	if (cnt_peers_is_ok(peers))
	{
		cnt_networking_segment_type ecs_segment = CNT_NETWORK_SEGMENT_TYPE_ECS;
		fck_serialise(&serialiser, &ecs_segment);

		if (cnt_peers_is_hosting(peers))
		{
			// Send to client - ALWAYS full state
			fck_ecs_snapshot_store(ecs, &serialiser);
		}
		else
		{
			// Send to host - Partial is ok
			fck_ecs::sparse_array<fck_authority> *entities = fck_ecs_view_single<fck_authority>(ecs);
			fck_ecs_snapshot_store_partial(ecs, &serialiser, &entities->owner);
		}
	}

	cnt_session_broadcast(session, serialiser.data, serialiser.at);

	fck_serialiser_free(&serialiser);
}

void cnt_networking_process_setup_new_connections(fck_ecs *ecs)
{
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);
	cnt_peers *peers = fck_ecs_unique_view<cnt_peers>(ecs);
	cnt_session_callbacks *callbacks = fck_ecs_unique_view<cnt_session_callbacks>(ecs);

	cnt_connection_handle handle;
	cnt_connection connection;
	while (cnt_session_try_dequeue_new_connection(session, &handle, &connection))
	{
		// callback required
		//
		// The requester is never responsible, the one accepting is responsible!
		bool is_hosting = (connection.flags & CNT_CONNECTION_FLAG_CLIENT) != CNT_CONNECTION_FLAG_CLIENT;
		if (is_hosting)
		{
			cnt_peer *peer;
			if (cnt_peers_try_add(peers, &handle, &peer))
			{
				if (callbacks != nullptr && callbacks->on_connect_as_host != nullptr)
				{
					cnt_on_connect_as_host_params data;
					data.ecs = ecs;
					data.peer = peer;
					data.connection = &connection;
					data.connection_handle = &handle;
					callbacks->on_connect_as_host(&data);
				}
			}
		}
		else
		{
			if (callbacks != nullptr && callbacks->on_connect_as_host != nullptr)
			{
				cnt_on_connect_as_client_params data;
				data.ecs = ecs;
				callbacks->on_connect_as_client(&data);
			}
		}
	}
}

void networking_process(fck_ecs *ecs, fck_system_update_info *)
{
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);
	fck_time *time = fck_ecs_unique_view<fck_time>(ecs);

	cnt_networking_process_recv(ecs);

	bool will_tick_this_frame = cnt_session_will_tick(session, time->delta);
	if (will_tick_this_frame)
	{
		cnt_networking_process_send(ecs);
	}

	cnt_session_tick(session, time->current, time->delta);

	if (will_tick_this_frame)
	{
		// Technically on a connection (from tick) we will first process networking events
		// AND THEN we will send the on connect setup message to the player
		// Man... I suck
		cnt_networking_process_setup_new_connections(ecs);
	}
}
