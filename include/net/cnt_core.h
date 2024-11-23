#ifndef CNT_CORE_INCLUDED
#define CNT_CORE_INCLUDED

#include "ecs/fck_ecs.h"
#include "net/cnt_peers.h"
#include "net/cnt_session.h"

enum cnt_networking_communication_mode : uint32_t
{
	CNT_NETWORK_COMMUNICATION_MODE_MESSAGE_UNICAST,
	CNT_NETWORK_COMMUNICATION_MODE_REPLICATION_BROADCAST
};

enum cnt_networking_segment_type : uint32_t
{
	CNT_NETWORK_SEGMENT_TYPE_ECS = 0xEC5,
	CNT_NETWORK_SEGMENT_TYPE_PEERS = 0xBEEF,
	CNT_NETWORK_SEGMENT_TYPE_EOF = 0xFFFF
};

struct cnt_welcome_message
{
	uint64_t id;

	cnt_peer_id self;
	cnt_peer_id host;

	fck_ecs::entity_type avatar;

	cnt_peer peer;
};

FCK_SERIALISE_OFF(fck_authority)
struct fck_authority
{
};

struct cnt_on_connect_as_host_params
{
	fck_ecs *ecs;
	cnt_peer *peer;
	cnt_connection *connection;
	cnt_connection_handle *connection_handle;
};

struct cnt_on_connect_as_client_params
{
	fck_ecs *ecs;
};

struct cnt_on_message_params
{
	fck_ecs *ecs;
	fck_serialiser *serialiser;
	cnt_connection_handle const *connection;
};

struct cnt_session_callbacks
{
	void (*on_connect_as_client)(cnt_on_connect_as_client_params const *in);
	void (*on_connect_as_host)(cnt_on_connect_as_host_params const *in);

	void (*on_message)(cnt_on_message_params const *in);
};

inline void fck_serialise(fck_serialiser *serialiser, cnt_networking_communication_mode *value)
{
	fck_serialise(serialiser, (uint32_t *)value);
}

inline void fck_serialise(fck_serialiser *serialiser, cnt_networking_segment_type *value)
{
	fck_serialise(serialiser, (uint32_t *)value);
}

inline void fck_serialise(fck_serialiser *serialiser, cnt_welcome_message *value)
{
	fck_serialise(serialiser, &value->id);
	fck_serialise(serialiser, &value->self);
	fck_serialise(serialiser, &value->host);

	fck_serialise(serialiser, &value->peer);
	fck_serialise(serialiser, &value->avatar);
}

void cnt_networking_process_recv_message_unicast(fck_ecs *ecs, fck_serialiser *serialiser, cnt_connection_handle const *connection);

void cnt_networking_process_recv_replication_broadcast(fck_ecs *ecs, fck_serialiser *serialiser, cnt_connection_handle const *connection);

void cnt_networking_process_recv(fck_ecs *ecs);

void cnt_networking_process_send(fck_ecs *ecs);

void cnt_networking_process_setup_new_connections(fck_ecs *ecs);

void networking_process(fck_ecs *ecs, fck_system_update_info *);

#endif // !CNT_CORE_INCLUDED
