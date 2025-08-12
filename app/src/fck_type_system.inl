#include "fck_type_system.h"

#include <fck_hash.h>

typedef struct fck_member_info
{
	fck_hash_int hash;
	fck_type owner;
	fck_identifier identifier;
	fck_type type;
	fckc_size_t stride;
	fck_member next;
} fck_member_info;

typedef struct fck_type_info
{
	fck_hash_int hash;
	fck_identifier identifier;
	fck_member first_member;
	fck_member last_member;
} fck_type_info;