#ifndef FCK_EC_H_INCLUDED
#define FCK_EC_H_INCLUDED

#include <fckc_inttypes.h>

#define fck_ec_api_name "fck-ec"

struct kll_allocator;

struct fck_ec_private;
typedef struct fck_ec
{
	struct fck_ec_private *opaque;
} fck_ec;

typedef enum fck_entity_bits
{
	fck_entity_alive_bit_count = 1,

	fck_entity_index_bit_count = 23,
	fck_entity_index_invalid = (1 << fck_entity_index_bit_count) - 1,

	fck_entity_generation_bit_count = 8,
	fck_entity_generation_max = (1 << fck_entity_generation_bit_count) - 1,
} fck_entity_bits;

typedef struct fck_entity
{
	fckc_u32 index : fck_entity_index_bit_count;
	fckc_u32 alive : fck_entity_alive_bit_count;
	fckc_u32 generation : fck_entity_generation_bit_count;
} fck_entity;

typedef struct fck_ec_entity_api
{
	fck_entity (*create)(fck_ec ec);
	int (*destroy)(fck_ec ec, fck_entity entity);
} fck_ec_entity_api;

typedef struct fck_ec_component_api
{
	int (*declare)(fck_ec ec, const char *name, fckc_u32 size);
	// TODO: (*define), so we can serialise each field!

	// TODO: Now we have access fully with name, maybe there is a better way!
	// Currently we work with stable capacity and stable indexing into the underlying components array (list of component arrays)
	// We can only declare and define, but we can never remove - by design
	// fck_component_id id = ec->component->resolve(ec, "name");
	// ec->component->set(ec, entity, id, NULL);
	// This could work!
	int (*set)(fck_ec ec, fck_entity entity, const char *name, const void *data);
	int (*remove)(fck_ec ec, fck_entity entity, const char *name);

	void *(*get)(fck_ec ec, fck_entity entity, const char *name);

	fckc_u32 (*dense)(fck_ec ec, const char *name, const fck_entity **values);
	void *(*buffer)(fck_ec ec, const char *name);
} fck_ec_component_api;

typedef struct fck_ec_core_api
{
	struct fck_ec (*create)(struct kll_allocator *allocator, fckc_u32 capacity);
	void (*destroy)(fck_ec ec);
} fck_ec_core_api;

typedef struct fck_ec_api
{
	// TODO: Debug
	// TODO: Archetype
	fck_ec_entity_api *entity;
	fck_ec_component_api *component;
	fck_ec_core_api *core;
} fck_ec_api;

#endif // !FCK_EC_H_INCLUDED
