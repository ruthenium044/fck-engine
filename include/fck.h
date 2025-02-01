#ifndef FCK_INCLUDED
#define FCK_INCLUDED

#include "core/fck_instances.h"

struct fck
{
	fck_instances instances;
};

void fck_init(fck *core, uint8_t instance_capacity);
void fck_prepare(fck *core, fck_instance_info const *info, fck_instance_setup_function setup_function);
int fck_run(fck *core);

#endif // !FCK_INCLUDED
