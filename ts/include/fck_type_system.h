#ifndef FCK_TYPE_SYSTEM_H_INCLUDED
#define FCK_TYPE_SYSTEM_H_INCLUDED

#include <fckc_inttypes.h>
#include <fckc_apidef.h>

#if defined(FCK_TYPE_SYSTEM_EXPORT)
#define FCK_TYPE_SYSTEM_API FCK_EXPORT_API
#else
#define FCK_TYPE_SYSTEM_API FCK_IMPORT_API
#endif
#define fck_name(s) #s

#define fck_id(s) sizeof(s) ? #s : NULL

struct kll_allocator;

struct fck_identifiers;
struct fck_types;
struct fck_members;
struct fck_marshal;

struct fck_type_info;
struct fck_member_info;
struct fck_assembly;

struct fck_serialiser;

typedef struct fck_identifier
{
	// This is ok, if we can guarantee a STABLE pointer
	struct fck_identifiers *identifiers;
	fckc_u64 hash;
} fck_identifier;

typedef struct fck_type
{
	// This is ok, if we can guarantee a STABLE pointer
	struct fck_types *types;
	fckc_u64 hash;
} fck_type;

typedef struct fck_member
{
	// This is ok, if we can guarantee a STABLE pointer
	struct fck_members *members;
	fckc_u64 hash;
} fck_member;

typedef struct fck_marshal_params
{
	// The previous params if recursive
	// struct fck_serialiser_params *parent;

	// Variable name - Makes sense
	const char *name;

	// Reference to the fck type... sure, can get used for tagging
	struct fck_type *type;

	// The type system iteself, else the type is useless - works...
	// Maybe it makes more sense to just hand around the type info itself?
	// struct fck_type_system *type_system;

	// TODO: Propagate parameters for special serialisers might be nice
	// We can also easily bind them...
	// void *user;
	//   ...
} fck_marshal_params;

typedef void(fck_marshal_func)(struct fck_serialiser *s, fck_marshal_params *p, void *self, fckc_size_t c);

typedef struct fck_identifier_desc
{
	const char *name;
} fck_identifier_desc;

typedef struct fck_type_desc
{
	const char *name;
	// We do not want to impose a size, but it is just too convenient...
	// This simply represents the layout size in C...
	// fckc_size_t size;
} fck_type_desc;

typedef struct fck_member_desc
{
	fck_type type;
	const char *name;
	fckc_size_t stride;
	fckc_size_t extra_count; // 1 + EXTRA
} fck_member_desc;

#define fck_value_decl(owner, type, member) (fck_member_desc){type, fck_name(member), offsetof(owner, member), 0}

#define fck_array_decl(owner, type, member, count)                                                                                         \
	(fck_member_desc)                                                                                                                      \
	{                                                                                                                                      \
		type, fck_name(member), offsetof(owner, member), ((count) - 1)                                                                     \
	}

// or list... idk yet
#define fck_dynarr_decl(owner, type, member)                                                                                             \
	(fck_member_desc)                                                                                                                      \
	{                                                                                                                                      \
		type, fck_name(member), offsetof(owner, member), (~0llu)                                                                           \
	}

inline fck_member_desc fck_member_desc_zero()
{
	return (fck_member_desc){0};
}

inline fck_member_desc fck_member_desc_value(fck_type type, const char *name, fckc_size_t stride)
{
	return (fck_member_desc){.type = type, .name = name, .stride = stride};
}

inline fck_member_desc fck_member_desc_array(fck_type type, const char *name, fckc_size_t stride, fckc_size_t count)
{
	// Count - 1 should break everything... let's see!!
	// ~0 can also mean nothing as a tombstone... we will see
	return (fck_member_desc){.type = type, .name = name, .stride = stride, .extra_count = count - 1};
}

typedef struct fck_marshal_desc
{
	// They just happen to be the same...
	fck_type type;
	fck_marshal_func *func;
} fck_marshal_desc;

typedef struct fck_identifier_api
{
	fck_identifier (*null)(void);
	int (*is_null)(fck_identifier identifier);
	int (*is_same)(fck_identifier a, fck_identifier b);
	const char *(*resolve)(fck_identifier identifier);
} fck_identifier_api;

// TODO: Nothing SHALL be string-based!! Except name ofc...
typedef struct fck_type_api
{
	fck_type (*null)(void);
	int (*is_null)(fck_type type);          // name: valid?
	int (*is_same)(fck_type a, fck_type b); // name:  same?
	int (*is)(fck_type a, const char *str);
	struct fck_type_info *(*resolve)(fck_type type);

	fck_identifier (*identify)(struct fck_type_info *info);
	fck_member (*members_of)(struct fck_type_info *info);
	// fckc_size_t (*size_of)(struct fck_type_info* info);

	fck_type (*add)(struct fck_assembly *assembly, fck_type_desc desc);
	fck_type (*get)(struct fck_assembly *assembly, fckc_u64 hash);
	fck_type (*find)(struct fck_assembly *assembly, const char *name);

	fckc_size_t (*size_of)(fck_type type);
	// Nice and convenient, but I need sorted
	int (*iterate)(struct fck_assembly *assembly, fck_type *type);

} fck_type_api;

typedef struct fck_member_api
{
	fck_member (*null)();
	int (*is_null)(fck_member member);
	int (*is_same)(fck_member a, fck_member b);

	struct fck_member_info *(*resolve)(fck_member member);

	fck_identifier (*identify)(struct fck_member_info *info);
	fck_type (*owner_of)(struct fck_member_info *info);
	fck_type (*type_of)(struct fck_member_info *info);
	fck_member (*next_of)(struct fck_member_info *info);
	fckc_size_t (*stride_of)(struct fck_member_info *info);
	fckc_size_t (*count_of)(struct fck_member_info *info);
	int (*is_dynarr)(struct fck_member_info *info);

	fck_member (*add)(fck_type owner, fck_member_desc desc);
} fck_member_api;

typedef struct fck_marshal_api
{
	void (*add)(fck_marshal_desc desc);
	// TODO: Maybe batched invoke? Let's do it later
	fck_marshal_func *(*get)(fck_type type);
} fck_marshal_api;

typedef struct fck_assembly_api
{
	struct fck_assembly *(*alloc)(struct kll_allocator *allocator);
	void (*free)(struct fck_assembly *assembly);
} fck_assembly_api;

typedef struct fck_type_system
{
	// Design choice: Pointers to api vs... not that
	fck_identifier_api *identifier;
	fck_type_api *type;
	fck_member_api *member;
	fck_marshal_api *marshal;
	fck_assembly_api *assembly;
} fck_type_system;

typedef struct fck_marshaller
{
	struct fck_serialiser *serialiser;
	struct fck_type_system *type_system;
} fck_marshaller;

struct fck_serialiser;
// struct fck_serialiser_params;
FCK_TYPE_SYSTEM_API void fck_type_serialise(fck_marshaller *serialiser, fck_marshal_params *params, void *data, fckc_size_t count);

struct fck_apis;

FCK_TYPE_SYSTEM_API fck_type_system *fck_load_type_system(struct fck_apis *apis);
FCK_TYPE_SYSTEM_API void fck_unload_type_system(struct fck_apis *apis);
FCK_TYPE_SYSTEM_API fck_type_system *fck_get_type_system(struct fck_apis *apis);

#endif // !FCK_TYPE_SYSTEM_H_INCLUDED
