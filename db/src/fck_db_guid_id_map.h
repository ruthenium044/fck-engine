#ifndef FCK_DB_GUID_ID_MAP_H_INCLUDED
#define FCK_DB_GUID_ID_MAP_H_INCLUDED

#include <fckc_atomic.h>
#include <fckc_inttypes.h>

typedef struct fck_db_guid
{
	// Will totally not get utilised!
	fckc_u64 time;
	fckc_u32 rand;
	// Can never be 0, maybe we hardcode 1 to in progress...
	fckc_atomic_u32 signal;
} fck_db_guid;

struct fck_db_guid_key_value;
typedef struct fck_db_guid_id_map
{
	struct fck_db_guid_key_value *values;
	fckc_size_t count;
	fckc_size_t capacity;
} fck_db_guid_id_map;

#endif // !FCK_DB_GUID_ID_MAP_H_INCLUDED
