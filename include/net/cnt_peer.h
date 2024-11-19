#ifndef CNT_PEER_INCLUDED
#define CNT_PEER_INCLUDED

#include "ecs/fck_serialiser.h"
#include <SDL3/SDL_stdinc.h>

enum cnt_peer_state : uint8_t
{
	cnt_peer_STATE_NONE,
	cnt_peer_STATE_CREATING,
	cnt_peer_STATE_OK
};

typedef uint16_t cnt_peer_id;

struct cnt_peer
{
	cnt_peer_id peer_id;
	// fck_ecs::entity_type avatar;
	cnt_peer_state state;
};

inline void fck_serialise(fck_serialiser *serialiser, cnt_peer_state *value)
{
	fck_serialise(serialiser, (uint8_t *)value);
}

inline void fck_serialise(fck_serialiser *serialiser, cnt_peer *value, size_t count = 1)
{
	for (int i = 0; i < count; i++)
	{
		fck_serialise(serialiser, &value->peer_id);
		fck_serialise(serialiser, &value->state);
	}
}

#endif // !CNT_PEER_INCLUDED
