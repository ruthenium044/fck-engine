#ifndef FCK_INSTANCES_INCLUDED
#define FCK_INSTANCES_INCLUDED

#include "fck_instance.h"

struct fck_instances
{
	fck_sparse_array<uint8_t, fck_instance> data;
	// If anything breaks, make sure to come up with a better mapping
	fck_dense_list<uint8_t, SDL_WindowID> pending_destroyed;
};

inline fck_iterator<fck_instance> begin(fck_instances *instances)
{
	SDL_assert(instances != nullptr);

	return begin(&instances->data.dense);
}

inline fck_iterator<fck_instance> end(fck_instances *instances)
{
	SDL_assert(instances != nullptr);

	return end(&instances->data.dense);
}

void fck_instances_alloc(fck_instances *instances, uint8_t capacity);

void fck_instances_free(fck_instances *instances);

bool fck_instances_any_active(fck_instances *instances);

fck_instance *fck_instances_add(fck_instances *instances, fck_instance_info const *info, fck_instance_setup_function instance_setup);

void fck_instances_process_events(fck_instances *instances);

#endif // !FCK_INSTANCES_INCLUDED