#ifndef FCK_TYPE_SYSTEM_H_INCLUDED
#define FCK_TYPE_SYSTEM_H_INCLUDED

#include <fckc_inttypes.h>

#define fck_name(s) #s

#define fck_id(s) sizeof(s) ? #s : NULL

struct fck_identifiers;
struct fck_types;
struct fck_members;
struct fck_serialise_interfaces;

struct fck_type_info;
struct fck_member_info;

struct fck_memory_serialiser;
struct fck_serialiser_params;

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

typedef void(fck_serialise_func)(struct fck_memory_serialiser *s, struct fck_serialiser_params *p, void *self, fckc_size_t c);

// Identifiers
// Let's make these public for now
fck_identifier fck_identifier_null();
int fck_identifier_is_null(fck_identifier identifier);
int fck_identifier_is_same(fck_identifier a, fck_identifier b);
const char *fck_identifier_resolve(fck_identifier identifier);

struct fck_identifiers *fck_identifiers_alloc(fckc_size_t capacity);
void fck_identifiers_free(struct fck_identifiers *ptr);

// Maybe these private cause only type and member are using them?
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
fck_identifier fck_type_info_identify(struct fck_type_info *info);
fck_member fck_type_info_first_member(struct fck_type_info *info);

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
fck_identifier fck_member_info_identify(struct fck_member_info *info);
fck_type fck_member_info_owner(struct fck_member_info* info);
fck_type fck_member_info_type(struct fck_member_info *info);
fck_member fck_member_info_next(struct fck_member_info *info);
fckc_size_t fck_member_info_stride(struct fck_member_info *info);

struct fck_members *fck_members_alloc(struct fck_identifiers *identifiers, fckc_size_t capacity);
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
struct fck_serialise_interfaces *fck_serialise_interfaces_alloc(fckc_size_t capacity);
void fck_serialise_interfaces_free(struct fck_serialise_interfaces *interfaces);

typedef struct fck_serialise_desc
{
	// They just happen to be the same...
	fck_type type;
	fck_serialise_func *func;
} fck_serialise_desc;
void fck_serialise_interfaces_add(struct fck_serialise_interfaces *interfaces, fck_serialise_desc desc);
fck_serialise_func *fck_serialise_interfaces_get(struct fck_serialise_interfaces *interfaces, fck_type type);

// Absolute base primitives
void fck_type_add_f32(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_f64(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_i8(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_i16(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_i32(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_i64(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_u8(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_u16(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_u32(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_u64(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);

// Combined primitives
void fck_type_add_f32x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_f32x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_f32x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);

void fck_type_add_f64x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_f64x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_f64x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);

void fck_type_add_i8x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_i8x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_i8x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);

void fck_type_add_i16x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_i16x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_i16x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);

void fck_type_add_i32x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_i32x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_i32x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);

void fck_type_add_i64x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_i64x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_i64x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);

void fck_type_add_u8x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_u8x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_u8x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);

void fck_type_add_u16x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_u16x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_u16x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);

void fck_type_add_u32x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_u32x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_u32x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);

void fck_type_add_u64x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_u64x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);
void fck_type_add_u64x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride);

// Setup!
void fck_type_system_setup_core(struct fck_types *types, struct fck_members *members, struct fck_serialise_interfaces *serialisers);

typedef struct fck_identifier_api
{
	fck_identifier (*null)(void);
	int (*is_null)(fck_identifier identifier);
	int (*is_same)(fck_identifier a, fck_identifier b);
	const char *(*resolve)(fck_identifier identifier);
} fck_identifier_api;

typedef struct fck_type_api
{
	fck_type (*null)(void);
	int (*is_null)(fck_type type);
	int (*is_same)(fck_type a, fck_type b);
	int (*is)(fck_type a, const char *str);
	struct fck_type_info *(*resolve)(fck_type type);

	fck_identifier (*identify)(struct fck_type_info *info);
	fck_member (*members_of)(struct fck_type_info *info);

	fck_type (*add)(fck_type_desc desc);
	fck_type (*find_from_hash)(fckc_u64 hash);
	fck_type (*find_from_string)(const char *name);

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

	fck_member (*push)(fck_member_desc desc);
} fck_member_api;

typedef struct fck_type_system
{
	fck_identifier_api *identifier;
	fck_type_api *type;
	fck_member_api *member;
} fck_type_system;

struct fck_apis;

void fck_load_type_system(struct fck_apis* apis);
void fck_unload_type_system(struct fck_apis* apis);
fck_type_system* fck_get_type_system(struct fck_apis* apis);

#endif // !FCK_TYPE_SYSTEM_H_INCLUDED
