#include "fck_db_undo.h"

#include "fck_db.h"

#include "fck_db_core.inl"

#include <kll.h>
#include <kll_malloc.h>
#include <fckc_inttypes.h>

#include <string.h>


static fck_db_undo_scope fck_db_undo_api_create(struct kll_allocator* allocator)
{
	fck_db_undo_scope result = { 0 };
	result.opaque = (fck_db_undo_scope_private*)kll_malloc(allocator, sizeof(*result.opaque));
	memset(result.opaque, 0, sizeof(*result.opaque));
	result.opaque->allocator = allocator;
	return result;
}

static void fck_db_undo_api_destroy(fck_db_undo_scope scope)
{
	kll_free(scope.opaque->allocator, scope.opaque);
}

static int fck_db_undo_api_undo(fck_db external, fck_db_undo_scope scope)
{
	fck_db_private* db = external.opaque;
	fck_db_undo_scope_private* undo_scope = scope.opaque;

	if (undo_scope->cursor == undo_scope->back)
	{
		return 0;
	}
	undo_scope->cursor = (undo_scope->cursor - 1) % fck_arraysize(undo_scope->units);

	fck_db_undo_unit* unit = undo_scope->units + undo_scope->cursor;

	fck_db_object* target = fck_db_resolve_object(db->page_table, unit->target);
	fck_db_object* copy = fck_db_resolve_object(db->page_table, unit->copy);

	const fck_db_object temp = *target;
	*target = *copy;
	*copy = temp;

	return 1;
}

static int fck_db_undo_api_redo(fck_db external, fck_db_undo_scope scope)
{
	fck_db_private* db = external.opaque;
	fck_db_undo_scope_private* undo_scope = scope.opaque;

	if (undo_scope->cursor == undo_scope->front)
	{
		return 0;
	}
	fck_db_undo_unit* unit = undo_scope->units + undo_scope->cursor;

	fck_db_object* target = fck_db_resolve_object(db->page_table, unit->target);
	fck_db_object* copy = fck_db_resolve_object(db->page_table, unit->copy);

	const fck_db_object temp = *target;
	*target = *copy;
	*copy = temp;

	undo_scope->cursor = (undo_scope->cursor + 1) % fck_arraysize(undo_scope->units);

	return 1;
}

static fck_db_undo_api db_undo_api = {
	.create = fck_db_undo_api_create,
	.destroy = fck_db_undo_api_destroy,
	.undo = fck_db_undo_api_undo,
	.redo = fck_db_undo_api_redo,
};

fck_db_undo_api* db_undo = &db_undo_api;