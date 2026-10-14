#include "my.h"

typedef struct my_assembly
{
	struct kll_arena* arena;
	const char* name;
	/* set */ my_type* types;
} my_assembly;