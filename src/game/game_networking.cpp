#include "game/game_systems.h"

#include "core/fck_instance.h"
#include "ecs/fck_ecs.h"
#include "net/cnt_core.h"

#include "game/game_components.h"
#include "game/game_core.h"

void networking_on_connect_as_host(cnt_on_connect_as_host_params const *in)
{
	fck_ecs *ecs = in->ecs;
	cnt_peer *peer = in->peer;
	cnt_connection_handle *handle = in->connection_handle;
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);

	// TODO: Remove this! Inject this on_server
	fck_ecs::entity_type avatar = game_cammy_create(ecs);

	uint8_t buffer[32];
	fck_serialiser serialiser;
	fck_serialiser_create(&serialiser, buffer, sizeof(buffer));
	fck_serialiser_byte_writer(&serialiser.self);

	// Header
	cnt_networking_communication_mode mode = CNT_NETWORK_COMMUNICATION_MODE_MESSAGE_UNICAST;
	fck_serialise(&serialiser, &mode);

	// Message
	cnt_welcome_message message;
	message.id = 0xB055;

	message.peer = *peer;
	message.avatar = avatar;
	fck_serialise(&serialiser, &message);

	cnt_session_send(session, handle, serialiser.data, serialiser.at);
	SDL_Log("Created Avatar: %d", avatar);
}

void networking_on_connect_as_client(cnt_on_connect_as_client_params const *in)
{
	fck_ecs *ecs = in->ecs;

	cnt_peers *peers = fck_ecs_unique_view<cnt_peers>(ecs);

	fck_ecs_entity_destroy_all(ecs);
	cnt_peers_clear(peers);
}

void networking_on_message(cnt_on_message_params const *in)
{
	fck_ecs *ecs = in->ecs;
	fck_serialiser *serialiser = in->serialiser;
	cnt_connection_handle const *connection = in->connection;

	cnt_peers *peers = fck_ecs_unique_view<cnt_peers>(ecs);

	// TODO: Messages need to get processed - This one only allows ONE
	cnt_welcome_message message;

	fck_serialise(serialiser, &message);

	// TODO: message.id should be message.type lol
	SDL_assert(message.id == 0xB055);

	// TODO: Remove this dependency!! Inject this on_client
	game_control_layout *layout = fck_ecs_component_create<game_control_layout>(ecs, message.avatar);
	*layout = {SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN};
	fck_ecs_component_create<cnt_authority>(ecs, message.avatar);

	message.peer.state = CNT_PEER_STATE_OK;

	cnt_peers_set_host(peers, message.host);
	cnt_peers_set_self(peers, message.peer.peer_id);

	cnt_peers_emplace(peers, &message.peer, connection);
}

void game_networking_setup(fck_ecs *ecs, fck_system_once_info *)
{
	fck_instance_info *info = fck_ecs_unique_view<fck_instance_info>(ecs);

	cnt_session *session = fck_ecs_unique_create<cnt_session>(ecs, cnt_session_free);
	cnt_peers *peers = fck_ecs_unique_create<cnt_peers>(ecs, cnt_peers_free);
	cnt_session_callbacks *callbacks = fck_ecs_unique_create<cnt_session_callbacks>(ecs);

	// TODO: Tidying this up would be nice - Inject it from outside?
	callbacks->on_connect_as_host = networking_on_connect_as_host;
	callbacks->on_connect_as_client = networking_on_connect_as_client;
	callbacks->on_message = networking_on_message;

	cnt_peers_alloc(peers);

	constexpr size_t tick_rate = 1000 / 60;
	cnt_session_alloc(session, 4, 128, 64, tick_rate);
	cnt_socket_handle socket_handle = cnt_session_socket_create(session, info->ip, info->source_port);
	if (info->destination_port != 0)
	{
		cnt_address_handle address_handle = cnt_session_address_create(session, info->ip, info->destination_port);
		cnt_session_connect(session, &socket_handle, &address_handle);
	}

	fck_ecs_system_add(ecs, networking_process);
}
