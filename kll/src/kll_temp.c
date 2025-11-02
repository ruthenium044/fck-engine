
#include "kll.h"
#include "kll_malloc.h"
#include <fckc_apidef.h>

#include <fckc_inttypes.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define kll_pointer_offset(type, ptr, offset) ((type *)(((fckc_u8 *)(ptr)) + (offset)))

typedef struct kll_temp_page
{
	fckc_size_t size;
	fckc_size_t capacity;
	struct kll_temp_page *next;
	// struct kll_temp_page *root;

	fckc_u8 *buffer;
} kll_temp_page;

typedef struct kll_temp_context
{
	kll_allocator *parent;
	kll_temp_page *pages;
	kll_temp_page root;
	// ...
} kll_temp_context;

static fckc_size_t fck_temp_allocator_align(fckc_size_t offset, fckc_size_t align)
{
	return (offset + align - 1) & ~(align - 1);
}

static void *kll_temp_realloc(kll_temp_context *context, void *ptr, fckc_size_t size, fckc_size_t line, const char *file)
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
	// When we realloc, we could poison the old memory, but I cannot be arsed
	size = fck_temp_allocator_align(size, sizeof(max_align_t));

	if (context->pages->size + size > context->pages->capacity)
	{
		fckc_size_t capacity = size * 8; // 8 i guess
		const fckc_size_t page_size = fck_temp_allocator_align(sizeof(*context->pages), sizeof(max_align_t));

		kll_temp_page *page = (kll_temp_page *)kll_malloc(context->parent, page_size + capacity);
		page->buffer = kll_pointer_offset(fckc_u8, context->pages, page_size);
		page->capacity = capacity;
		page->size = 0;
		page->next = context->pages;
		context->pages = page;
	}

	void *memory = kll_pointer_offset(void, context->pages->buffer, size);
	context->pages->size = context->pages->size + size;
	return memory;
}

void kll_temp_reset(kll_temp_context *context)
{
	// If we only have one page, just reset it
	kll_temp_page *root = &context->root;
	if (context->pages->next == root)
	{
		context->pages->size = 0;
		return;
	}

	// If we have multiple pages, aggregate them
	fckc_size_t suggested_size = context->root.size;
	kll_temp_page *page = context->pages;
	while (page != root)
	{
		suggested_size = suggested_size + page->size;
		kll_temp_page *next = page->next;
		kll_free(context->parent, page);
		page = next;
	}

	kll_free(context->parent, root->buffer);

	// Create a new page with the knowledge from before!
	// Maybe we should not do that here, instead we do it on first allocation
	// and remember the suggested size? Meh, this only happens when we go in page or slab realm
	const fckc_size_t buffer_size = fck_temp_allocator_align(suggested_size, sizeof(max_align_t));
	root->buffer = (fckc_u8 *)kll_malloc(context->parent, buffer_size);
	page->capacity = buffer_size;
	page->size = 0;
	page->next = root;
	context->pages = root;
}

char *kll_temp_allocator_format(kll_temp_context *context, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	va_list args_copy;
	va_copy(args_copy, args);

	int len = vsnprintf(NULL, 0, format, args_copy);
	va_end(args_copy);

	char *buffer = NULL;
	if (len >= 0)
	{
		buffer = (char *)kll_temp_realloc(context, NULL, len + 1, __LINE__, __FILE__);
		if (buffer)
		{
			vsnprintf(buffer, len + 1, format, args);
		}
	}

	va_end(args);
	return buffer;
}

FCK_EXPORT_API kll_temp_allocator *kll_temp_allocator_create(kll_allocator *parent, fckc_size_t capacity)
{
	kll_temp_context *context;
	kll_temp_allocator *temp;

	capacity = fck_temp_allocator_align(capacity, sizeof(max_align_t));

	const fckc_size_t context_size = fck_temp_allocator_align(sizeof(*context), sizeof(max_align_t));
	const fckc_size_t allocator_size = fck_temp_allocator_align(sizeof(*temp), sizeof(max_align_t));

	fckc_u8 *allocator_memory = (fckc_u8 *)kll_malloc(parent, context_size + allocator_size);
	temp = kll_pointer_offset(kll_temp_allocator, allocator_memory, 0);
	context = kll_pointer_offset(kll_temp_context, allocator_memory, allocator_size);
	context->parent = parent;

	fckc_u8 *buffer = (fckc_u8 *)kll_malloc(parent, capacity);
	context->root.buffer = buffer;
	context->root.next = &context->root;
	context->root.size = 0;
	context->root.capacity = capacity;
	context->pages = &context->root;
	temp->context = (kll_context *)context;
	// vt
	temp->temp.reset = (kll_reset_func *)kll_temp_reset;
	temp->temp.format = (kll_format_func *)kll_temp_allocator_format;
	temp->vt.realloc = (kll_realloc_function *)kll_temp_realloc;
	return temp;
}

FCK_EXPORT_API void kll_temp_allocator_destroy(kll_temp_allocator *temp)
{
	kll_temp_context *context = (kll_temp_context *)temp->context;
	kll_temp_page *root = &context->root;
	kll_temp_page *page = context->pages;
	while (page != root)
	{
		kll_free(context->parent, page);
		page = page->next;
	}
	kll_free(context->parent, context->root.buffer);
	kll_free(context->parent, temp);
}
