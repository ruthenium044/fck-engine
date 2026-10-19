#include <fckc_inttypes.h>

#define fck_my_api_name "my"

struct kll_allocator;
struct kll_arena;

struct my_type;
struct my_field;
typedef struct my_field
{
	const struct my_type  *type;
	const char            *name;
	fckc_size_t            offset;
	const struct my_field *next;
} my_field;

struct my_assembly;
typedef struct my_type
{
	fckc_u64                  hash;
	const struct my_assembly *assembly;
	const char               *name;
	fckc_size_t               size;
	const my_field           *fields;
	const my_field           *last;
} my_type;

typedef struct my_primitives
{
	const my_type *i8;
	const my_type *i16;
	const my_type *i32;
	const my_type *i64;
	const my_type *u8;
	const my_type *u16;
	const my_type *u32;
	const my_type *u64;
	const my_type *f32;
	const my_type *f64;
} my_primitives;

typedef struct my_api
{
	const struct my_primitives *primitives;
	// Let's keep it singular for now...
	const struct my_assembly   *system;

	const my_type *(*type)(const struct my_assembly *assembly, const char *name);

	my_type  *(*create_type)(const struct my_assembly *assembly, fckc_size_t size, const char *name);
	my_field *(*add_field)(my_type *type, fckc_size_t offset, const my_type *field_type, const char *name);
} my_api;

#define my_typeof(my, assembly, data_type) my->type((assembly), fck_nameof(data_type))