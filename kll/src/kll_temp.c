
#include "kll.h"
#include "kll_malloc.h"
#include <fckc_apidef.h>

#include <fckc_inttypes.h>

#define kll_pointer_offset(type, ptr, offset) ((type *)(((fckc_u8 *)(ptr)) + (offset)))

typedef struct kll_temp_page
{
	fckc_size_t size;
	fckc_size_t capacity;
	struct kll_temp_page *prev;
	fckc_u8 *buffer;
} kll_temp_page;

typedef struct kll_temp_context
{
	kll_allocator *fallback;

	kll_temp_page *pages;
	// ...
} kll_temp_context;

static fckc_size_t fck_temp_allocator_align(fckc_size_t offset, fckc_size_t align)
{
	return (offset + align - 1) & ~(align - 1);
}

static void *temp_realloc(kll_temp_context *context, void *ptr, fckc_size_t size, fckc_size_t line, const char *file)
{
	if (ptr == NULL && size == 0)
	{
		return NULL;
	}

	if (size == 0)
	{
		return NULL;
	}
	// In any case we allocate new
	// We could realloc and poison the old memory, but I cannot be arsed
	size = fck_temp_allocator_align(size, sizeof(max_align_t));

	if (context->pages == NULL || (context->pages->size + size) > context->pages->capacity)
	{
		fckc_size_t capacity = size * 8;
		const fckc_size_t page_size = fck_temp_allocator_align(sizeof(*context->pages), sizeof(max_align_t));

		kll_temp_page *page = (kll_temp_page *)kll_malloc(context->fallback, page_size + capacity);
		page->buffer = kll_pointer_offset(fckc_u8, context->pages, page_size);
		page->capacity = capacity;
		page->size = 0;
		page->prev = context->pages;
		context->pages = page;
	}

	void *memory = kll_pointer_offset(void, context->pages->buffer, size);
	context->pages->size = context->pages->size + size;
	return memory;
}

void temp_reset(kll_temp_context *context)
{
	// If we only have one page, just reset that one
	if (context->pages != NULL && context->pages->prev == NULL)
	{
		context->pages->size = 0;
	}

	// If we have multiple pages, aggregate them
	fckc_size_t suggested_size = 0;
	kll_temp_page *page = context->pages;
	while (page != NULL)
	{
		suggested_size = suggested_size + page->size;
		kll_free(context->fallback, page);
		page = page->prev;
	}

	// Create a new page with the knowledge from before!
	// Maybe we should not do that here, instead we do it on first allocation?
	const fckc_size_t page_size = fck_temp_allocator_align(sizeof(*context->pages), sizeof(max_align_t));
	page = (kll_temp_page *)kll_malloc(context->fallback, page_size + suggested_size);
	page->buffer = kll_pointer_offset(fckc_u8, context->pages, page_size);
	page->capacity = suggested_size;
	page->size = 0;
	page->prev = NULL;
	context->pages = page;
}

kll_temp_allocator *kll_temp_allocator_create(kll_temp_allocator *temp_memory, kll_allocator *fallback)
{
	temp_memory->temp.reset = (kll_reset_func *)temp_reset;
	temp_memory->vt.realloc = (kll_realloc_function *)temp_realloc;

	return temp_memory;
}

static kll_temp_allocator temp_allocator = (kll_temp_allocator){
	.context = NULL, .vt.realloc = (kll_realloc_function *)temp_realloc,
	//.temp = &temp_temp_vt,
};

FCK_EXPORT_API kll_temp_allocator *kll_temp = &temp_allocator;