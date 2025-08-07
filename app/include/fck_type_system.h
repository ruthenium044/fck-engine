#ifndef FCK_TYPE_SYSTEM_H_INCLUDED
#define FCK_TYPE_SYSTEM_H_INCLUDED

#include <fckc_inttypes.h>

struct fck_identifiers;
struct fck_types;
struct fck_members;
struct fck_serialise_interfaces;
struct fck_type_info;
struct fck_member_info;

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

// TODO: come up with a better name... This is actually shit
// TODO: Test if we can pointer cast this one a la me-style in emscripten
// TODO: Might need resolve? What if we get interface and then we simply exchange it during runtime
// Keeping it open addressed COULD be better... Maybe run the call through type_ysten TU even?
typedef void (*fck_serialise_i_func)(void *self, fckc_size_t c, struct fck_serialiser *s, struct fck_serialiser_params *p);
typedef struct fck_serialise_i
{
	// Maybe...
	// fck_type type;
	fck_serialise_i_func func;
} fck_serialise_i;

// Maybe prefer:
//typedef struct fck_serialise_i2
//{
//	struct fck_serialise_interfaces;
//	// fck_type type; // Maybe instead of hash?
//	fckc_u64 hash;
//} fck_serialise_i2;
// fck_serialise_i_func fck_serialise_i2_resolve(fck_serialise_i2 interface);
//  or
// void fck_serialise_i2_invoke(fck_serialise_i2 interface, void *self, fckc_size_t c, struct fck_serialiser *s,
//                             struct fck_serialiser_params *p);

// Identifiers
fck_identifier fck_identifier_null();
int fck_identifier_is_null(fck_identifier identifier);
int fck_identifier_is_same(fck_identifier a, fck_identifier b);
const char *fck_identifier_resolve(fck_identifier identifier);

fck_identifiers *fck_identifiers_alloc();
void fck_identifiers_free(fck_identifiers *ptr);

typedef struct fck_identifier_desc
{
	const char *name;
} fck_identifier_desc;
fck_identifier fck_identifiers_add(struct fck_identifiers *identifiers, fck_identifier_desc desc);
fck_identifier fck_identifiers_find_from_hash(struct fck_identifiers *identifiers, fckc_u64 hash);
fck_identifier fck_identifiers_find_from_string(struct fck_identifiers *identifiers, const char *str);

// Types
fck_type fck_type_null();
int fck_type_is_null(fck_type type);
int fck_type_is_same(fck_type a, fck_type b);
int fck_type_is(fck_type a, const char *str);
struct fck_type_info *fck_type_resolve(fck_type type);

struct fck_types *fck_types_alloc(struct fck_identifiers *identifiers, fckc_size_t capacity);
void fck_types_free(struct fck_types *types);

typedef struct fck_type_desc
{
	const char *name;
} fck_type_desc;
fck_type fck_types_add(struct fck_types *types, fck_type_desc desc);
fck_type fck_types_find_from_hash(struct fck_types *types, fckc_u64 hash);
fck_type fck_types_find_from_string(struct fck_types *types, const char *name);

// Members
fck_member fck_member_null();
int fck_member_is_null(fck_member member);
int fck_member_is_same(fck_member a, fck_member b);
struct fck_member_info *fck_member_resolve(fck_member member);

fck_members *fck_member_registry_alloc(struct fck_identifiers *identifiers, fckc_size_t capacity);
void fck_members_free(struct fck_members *members);

typedef struct fck_member_desc
{
	fck_type type;
	fck_type owner;
	const char *name;
	fckc_size_t stride;
} fck_member_desc;
fck_member fck_members_add(struct fck_members *members, fck_member_desc desc);

// Serialisers
struct fck_serialiser_interfaces *fck_serialiser_interfaces_alloc(fckc_size_t capacity);
void fck_serialiser_interfaces_free(struct fck_serialiser_interfaces *interfaces);

typedef struct fck_serialise_desc
{
	// They just happen to be the same...
	fck_type type;
	fck_serialise_i_func func;
} fck_serialise_desc;
void fck_serialiser_interfaces_add(struct fck_serialiser_interfaces *interfaces, fck_serialise_desc desc);
// Kind of breaks semantics... "get" -> prefer resolve?
fck_serialise_i fck_serialiser_interfaces_get(struct fck_serialiser_interfaces *interfaces, fck_type type);

#endif // !FCK_TYPE_SYSTEM_H_INCLUDED
