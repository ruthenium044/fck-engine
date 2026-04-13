
#include "kll.h"
#include "kll_system.h"

static struct kll_arena_api kll_arena_api;

struct kll_api kll_api = (struct kll_api){
	.arena = &kll_arena_api,
};

