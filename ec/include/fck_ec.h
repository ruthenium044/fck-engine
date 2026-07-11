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

//  Might need to give these a namespace at some point fck_ec_...
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

typedef struct fck_component_id
{
	fckc_uintptr value;
} fck_component_id;

typedef struct fck_component_definition
{
	// Triggered on component add (Implicitly invoked by ec::component->set)
	int (*constructor)(void *self, void *userdata);
	// Triggered on component remove (Explicitly invoked by ec::component->remove)
	int (*destructor)(void *self, void *userdata);

	void *userdata;
} fck_component_definition;

typedef struct fck_query_component
{
	fck_component_id id;
	fckc_u32 offset;
} fck_query_component;

typedef struct fck_query_description
{
	const char *name;
	const fck_query_component *components;
	fckc_u32 size;
	fckc_u32 count;
} fck_query_description;

typedef struct fck_query_id
{
	void *opaque;
} fck_query_id;

typedef struct fck_query_iterator
{
	fck_ec ec;
	fck_query_id query;
	fckc_u32 index;
} fck_query_iterator;

typedef struct fck_archetype_iterator
{
	fck_ec ec;
	fck_entity entity;
	void *opaque;
} fck_archetype_iterator;

typedef struct fck_component_names_iterator
{
	fck_ec ec;
	void *opaque;
} fck_component_names_iterator;

typedef struct fck_ec_archetype_api
{
	fck_archetype_iterator (*iterator)(fck_ec ec, fck_entity entity);
	fckc_u32 (*get)(fck_archetype_iterator *it, fck_component_id *components, fckc_u32 capacity);
} fck_ec_archetype_api;

typedef struct fck_ec_entity_api
{
	fckc_u32 (*all)(fck_ec ec, const fck_entity **values);

	fck_entity (*create)(fck_ec ec);
	int (*destroy)(fck_ec ec, fck_entity entity);
} fck_ec_entity_api;

typedef struct fck_ec_component_api
{
	// data can be NULL -> zeroes out memory
	int (*set)(fck_ec ec, fck_entity entity, fck_component_id id, const void *data);
	int (*remove)(fck_ec ec, fck_entity entity, fck_component_id id);
	void *(*get)(fck_ec ec, fck_entity entity, fck_component_id id);

	fckc_u32 (*dense)(fck_ec ec, fck_component_id id, const fck_entity **values);
	void *(*buffer)(fck_ec ec, fck_component_id id);
} fck_ec_component_api;

typedef struct fck_ec_registry_api
{
	fck_component_id (*declare)(fck_ec ec, const char *name, fckc_u32 size);
	// TODO: (*define), so we can serialise each field!
	int (*define)(fck_ec ec, fck_component_id id, const fck_component_definition *definition);

	fck_component_id (*id)(fck_ec ec, const char *name);
	const char *(*nameof)(fck_ec ec, fck_component_id id);

	fck_component_names_iterator (*iterator)(fck_ec ec);
	fckc_u32 (*names)(fck_component_names_iterator *it, const char **names, fckc_u32 capacity);
} fck_ec_registry_api;

typedef struct fck_ec_query_api
{
	fck_query_id (*get)(fck_ec ec, const fck_query_description *description);
	fck_query_iterator (*iterator)(fck_ec ec, fck_query_id query);
	fckc_u32 (*match)(fck_query_iterator *it, void *dst, fckc_u32 capacity);
} fck_ec_query_api;

// typedef struct fck_ec_debug_api
//{
//
//
// } fck_ec_debug_api;

typedef struct fck_ec_core_api
{
	struct fck_ec (*create)(struct kll_allocator *allocator, fckc_u32 capacity);
	void (*destroy)(fck_ec ec);
} fck_ec_core_api;

// I am inclined to prefer the vocabulary "Database"
typedef struct fck_ec_api
{
	// TODO: Debug
	fck_ec_archetype_api *archetype;
	fck_ec_registry_api *registry;
	fck_ec_query_api *query;
	fck_ec_entity_api *entity;
	fck_ec_component_api *component;
	fck_ec_core_api *core;
} fck_ec_api;

#endif // !FCK_EC_H_INCLUDED
