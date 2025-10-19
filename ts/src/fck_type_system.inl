#include "fck_type_system.h"

#include <fck_hash.h>

// Identifiers
// Let's make these public for now
fck_identifier fck_identifier_null(void);
int fck_identifier_is_null(fck_identifier identifier);
int fck_identifier_is_same(fck_identifier a, fck_identifier b);
const char *fck_identifier_resolve(fck_identifier identifier);

void fck_identifiers_alloc(struct fck_identifiers *identifiers, struct fck_assembly *assembly, fckc_size_t capacity);
void fck_identifiers_free(struct fck_identifiers *ptr);

fck_identifier fck_identifiers_add(struct fck_identifiers *identifiers, fck_identifier_desc desc);
fck_identifier fck_identifiers_find_from_hash(struct fck_identifiers *identifiers, fckc_u64 hash);
fck_identifier fck_identifiers_find_from_string(struct fck_identifiers *identifiers, const char *str);

// Types
fck_type fck_type_null(void);
int fck_type_is_null(fck_type type);
int fck_type_is_same(fck_type a, fck_type b);
int fck_type_is(fck_type a, const char *str);

struct fck_type_info *fck_type_resolve(fck_type type);
fck_identifier fck_type_info_identify(struct fck_type_info *info);
fck_member fck_type_info_first_member(struct fck_type_info *info);

void fck_types_alloc(struct fck_types *types, struct fck_assembly *assembly, struct fck_identifiers *identifiers, fckc_size_t capacity);
void fck_types_free(struct fck_types *types);

struct fck_assembly *fck_types_assembly(struct fck_types *types);

fck_type fck_types_add(struct fck_types *types, fck_type_desc desc);
fck_type fck_types_find_from_hash(struct fck_types *types, fckc_u64 hash);
fck_type fck_types_find_from_string(struct fck_types *types, const char *name);

int fck_types_iterate(struct fck_types *types, fck_type *type);

// Members
fck_member fck_member_null(void);
int fck_member_is_null(fck_member member);
int fck_member_is_same(fck_member a, fck_member b);

struct fck_member_info *fck_member_resolve(fck_member member);
fck_identifier fck_member_info_identify(struct fck_member_info *info);
fck_type fck_member_info_owner(struct fck_member_info *info);
fck_type fck_member_info_type(struct fck_member_info *info);
fck_member fck_member_info_next(struct fck_member_info *info);
fckc_size_t fck_member_info_stride(struct fck_member_info *info);
fckc_size_t fck_member_info_count(struct fck_member_info *info);

void fck_members_alloc(struct fck_members *members, struct fck_assembly *assembly, struct fck_identifiers *identifiers,
                       fckc_size_t capacity);
void fck_members_free(struct fck_members *members);

fck_member fck_members_add(struct fck_members *members, fck_type owner, fck_member_desc desc);

// Marshal
void fck_marshal_alloc(struct fck_marshal *marshal, struct fck_assembly *assembly, fckc_size_t capacity);
void fck_marshal_free(struct fck_marshal *interfaces);

void fck_marshal_add(struct fck_marshal *interfaces, fck_marshal_desc desc);
fck_marshal_func *fck_marshal_get(struct fck_marshal *interfaces, fck_type type);

typedef struct fck_member_info
{
	// Who owns it?
	fck_type owner;
	// The identifier (var name) of member
	fck_identifier identifier;
	// The actual type of member
	fck_type type;
	// Stride, where to start when getting incoming void* data
	fckc_size_t stride;

	// extra_count == 0 stays valid, yay
	fckc_size_t extra_count;

	// All this data so far here is rather static...
	// We could add a dynamic section
	// But instead of binding to data, we bind to... idk, kll with void*?
	// Maybe implementing a fixed_array is ok for now?
	// We cannot live with type system info alone!!!!! We need to establish protocol/header info

	// Next member
	fck_member next;
} fck_member_info;

typedef struct fck_type_info
{
	// fck_hash_int hash;
	fck_identifier identifier;

	// Here comes the killer thing... They are NOT ordered by stride. Oh fuck me...
	fck_member first_member;
	fck_member last_member;
} fck_type_info;

typedef struct fck_identifiers
{
	struct fck_assembly *assembly;

	/* set */ struct fck_identifier_info *info;
} fck_identifiers;

typedef struct fck_types
{
	struct fck_assembly *assembly;
	struct fck_identifiers *identifiers;

	/* set */ struct fck_type_info *info;
} fck_types;

typedef struct fck_members
{
	struct fck_assembly *assembly;
	struct fck_identifiers *identifiers;

	/* set */ struct fck_member_info *info;
} fck_members;

typedef struct fck_marshal
{
	struct fck_assembly *assembly;

	/* set */ struct fck_marshal_info *infoo;
} fck_marshal;

struct kll_allocator;

typedef struct fck_assembly
{
	struct kll_allocator *allocator;
	fck_identifiers identifiers;
	fck_types types;
	fck_members members;
	fck_marshal marshal;
} fck_assembly;

void fck_type_system_setup_core(struct fck_types *types, struct fck_members *members, struct fck_marshal *serialisers);

void fck_serialise_identifier(struct fck_serialiser *serialiser, struct fck_marshal_params *params, fck_identifier *v, fckc_size_t c);
void fck_serialise_type(struct fck_serialiser *serialiser, struct fck_marshal_params *params, fck_type *v, fckc_size_t c);
void fck_serialise_member(struct fck_serialiser *serialiser, struct fck_marshal_params *params, fck_member *v, fckc_size_t c);
void fck_serialise_type_info(struct fck_serialiser *serialiser, struct fck_marshal_params *params, fck_type_info *v, fckc_size_t c);
void fck_serialise_member_info(struct fck_serialiser *serialiser, struct fck_marshal_params *params, fck_member_info *v, fckc_size_t c);

void fck_serialise_identifiers(struct fck_serialiser *serialiser, struct fck_marshal_params *params, fck_identifiers *v, fckc_size_t c);
void fck_serialise_types(struct fck_serialiser *serialiser, struct fck_marshal_params *params, fck_types *v, fckc_size_t c);
void fck_serialise_members(struct fck_serialiser *serialiser, struct fck_marshal_params *params, fck_members *v, fckc_size_t c);