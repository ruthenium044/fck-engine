
#include "kll.h"
#include "kll_heap.h"
#include "kll_malloc.h"
#include "kll_scratch.h"
#include "fckc_inttypes.h"

typedef struct list_int
{
	kll_allocator *allocator;
	int *data; 
	size_t count;
	size_t capacity;
} list_int;

void list_int_constructor(list_int *this, kll_allocator *allocator, size_t capacity) // decl and alloc this of size (list_int ptr), allocator of size (allocator ptr) and capacity of size (size_t -> some integer)
{
	// decl -> declare variable of type
	// allc -> allocate variable
	// assign -> assigns rhs to lhs (lhs = rhs)
	// jump -> function call, statement, anything that breaks linear code.

	this->allocator = allocator; // assigns allocator to this->allocator
	this->capacity = capacity; // assigns capacity to this->capacity
	this->data = kll_malloc(allocator, capacity * sizeof(int)); // allocates data and then assigns it to this->data
	this->count = 0; // assigns 0 to this->count
};

void list_int_clear(list_int *this)
{
	this->count = 0;
};

void list_int_destructor(list_int *this)
{
	kll_free(this->allocator, this->data);
	this->count = 0;
	this->capacity = 0;
	this->data = (int *)0xBEEFBEEF;
	this->allocator = (kll_allocator *)0xBEEFBEEF;
}

typedef struct assets
{
	list_int images[16];
} assets;

void assets_destroy(assets *this)
{
	for (int i = 0; i < 16; i++)
	{
		list_int_destructor(&this->images[i]);
	}
}


int main(int argc, char **argv)
{
	{
		// decl -> declare variable of type
		// allc -> allocate variable
		// assign -> assigns rhs to lhs (lhs = rhs)
		// jump -> function call, statement, anything that breaks linear code.

		assets asses; // decl asses of type assets, allocate of size(assets)
					  // decl asses members, and asses members mebers
		list_int initial_list;// decl list of type list_ist, allocate of size(list_int)

		list_int_constructor(&initial_list, kll_heap, 128); // jump to list_int_constructor
		
		for (int i = 0; i < 128; i++)
		{
			// Allocator* = MemAddress
			// count = N
			// capacity = 128
			// Data 0, 1, 2, 3, 4, 5, 6, 7, 8, ... 127
			initial_list.data[i] = i;
		}
		for (int i = 0; i < 16; i++)
		{
			// list to asses.images[i]
			// - assign data, count and capacity to asses.images[i]
			// - pointer is the problem 
			// - new pointer, and assign data... to new pointer data...
			// - pointer points to the same 128 elements as incoming list
			// - the solution should NOT do that -> copy :-)
			asses.images[i] = initial_list;
		}

		// Assign 0 to first element of data OF first image(list) of assets
		asses.images[0].data[0] = 0;
		// They are the same -> it should be false cause we want copy
		int /*bool*/ result = asses.images[0].data[0] == asses.images[1].data[0] == asses.images[2].data[0]; // and so on
		
		// Kill it. Data gets beefed
		list_int_destructor(&initial_list);

		// Crash -> crash on data
		asses.images[0].data[0] = 0;
	}

	// void *memory = kll_malloc(kll_heap, 1024);
	unsigned char stack[1024];
	kll_allocator *scratch = kll_scratch_create(stack, sizeof(stack), 8);
	for (int i = 0; i < 16; i++)
	{
		kll_malloc(scratch, 128);
		kll_scratch_reset(scratch);
	}

	// kll_free(kll_heap, memory);
	return 0;
}