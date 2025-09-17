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

struct fck_serialiser;
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

typedef void(fck_serialise_func)(struct fck_serialiser *s, struct fck_serialiser_params *p, void *self, fckc_size_t c);

typedef struct fck_identifier_desc
{
	const char *name;
} fck_identifier_desc;

typedef struct fck_type_desc
{
	const char *name;
	// We do not want to impose a size, but it is just too convenient...
	// This simply represents the layout size in C...
	fckc_size_t size;
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

typedef struct fck_serialise_desc
{
	// They just happen to be the same...
	fck_type type;
	fck_serialise_func *func;
} fck_serialise_desc;

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
	int (*is_null)(fck_type type);          // name: valid?
	int (*is_same)(fck_type a, fck_type b); // name:  same?
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
	fckc_size_t (*count_of)(struct fck_member_info *info);

	fck_member (*add)(fck_type owner, fck_member_desc desc);
} fck_member_api;

typedef struct fck_serialise_interface_api
{
	void (*add)(fck_serialise_desc desc);
	// TODO: Maybe batched invoke? Let's do it later
	fck_serialise_func *(*get)(fck_type type);

} fck_serialise_interface_api;

typedef struct fck_primitive_api
{
	void (*add_f32)(fck_type owner, const char *name, fckc_size_t stride);
	void (*add_f64)(fck_type owner, const char *name, fckc_size_t stride);
	void (*add_i8)(fck_type owner, const char *name, fckc_size_t stride);
	void (*add_i16)(fck_type owner, const char *name, fckc_size_t stride);
	void (*add_i32)(fck_type owner, const char *name, fckc_size_t stride);
	void (*add_i64)(fck_type owner, const char *name, fckc_size_t stride);
	void (*add_u8)(fck_type owner, const char *name, fckc_size_t stride);
	void (*add_u16)(fck_type owner, const char *name, fckc_size_t stride);
	void (*add_u32)(fck_type owner, const char *name, fckc_size_t stride);
	void (*add_u64)(fck_type owner, const char *name, fckc_size_t stride);

	void (*add)(fck_member_desc desc);

} fck_primitive_api;

typedef struct fck_type_system
{
	// TODO: Remove these, they just bandaid atm
	struct fck_identifiers *(*get_identifiers)(void);
	struct fck_types *(*get_types)(void);
	struct fck_members *(*get_members)(void);
	struct fck_serialise_interfaces *(*get_serialisers)(void);

	// Design choice: Pointers to api vs... not that
	fck_identifier_api *identifier;
	fck_type_api *type;
	fck_member_api *member;
	fck_serialise_interface_api *serialise;
} fck_type_system;

struct fck_apis;

void fck_load_type_system(struct fck_apis *apis);
void fck_unload_type_system(struct fck_apis *apis);
fck_type_system *fck_get_type_system(struct fck_apis *apis);

#endif // !FCK_TYPE_SYSTEM_H_INCLUDED
