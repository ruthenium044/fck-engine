#include "sad_pointer_queue.h"
#include "sad_atomic.h"
#include "sad_thread.h"

#include <memory.h>
#include <stdio.h>

#define SAD_CRASH(condition)                                                                                                               \
	if ((condition))                                                                                                                       \
		((*(char *)0) = 1);

typedef struct sad_pointer_queue_write_descriptor
{
	void *value;
	struct sad_pointer_queue_write_descriptor *next;
	fckc_u64 owner;
	struct sad_concurrent_queue_write_desciptor_block *block;
	fckc_u8 padding[28];
} sad_pointer_queue_write_descriptor;

typedef struct sad_concurrent_queue_write_desciptor_block
{
	struct sad_concurrent_queue_write_desciptor_block *prev;
	struct sad_concurrent_queue_write_desciptor_block *next_free;
	fckc_size_t capacity;
	fckc_size_t used;
	fckc_size_t unused;
	sad_pointer_queue_write_descriptor buffer[1];
} sad_concurrent_queue_write_desciptor_block;

typedef struct sad_concurrent_queue_write_desciptor_thread_local
{
	fckc_u64 thread_id;
	sad_concurrent_queue_write_desciptor_block *current;
	sad_concurrent_queue_write_desciptor_block *free;
	sad_concurrent_queue_write_desciptor_block *tail;
} sad_concurrent_queue_write_desciptor_thread_local;

typedef struct sad_concurrent_queue_write_desciptor_allocator
{
	sad_concurrent_queue_write_desciptor_thread_local storages[32];
} sad_concurrent_queue_write_desciptor_allocator;

sad_concurrent_queue_write_desciptor_block *sad_concurrent_queue_write_desciptor_block_alloc(sad_pointer_queue_allocator *allocator,
                                                                                             fckc_size_t capacity)
{
	const fckc_size_t header_size = sizeof(sad_concurrent_queue_write_desciptor_block);
	const fckc_size_t element_size = sizeof(sad_pointer_queue_write_descriptor) * capacity - 1;
	void *memory = allocator->realloc(allocator->context, NULL, header_size + element_size);
	sad_concurrent_queue_write_desciptor_block *block = (sad_concurrent_queue_write_desciptor_block *)memory;
	block->capacity = capacity;
	block->used = 0;
	block->unused = 0;
	block->next_free = NULL;
	block->prev = NULL;
	return block;
}

void sad_concurrent_queue_write_desciptor_block_free(sad_pointer_queue_allocator *allocator,
                                                     sad_concurrent_queue_write_desciptor_block *block)
{
	allocator->realloc(allocator->context, block, 0);
}

sad_concurrent_queue_write_desciptor_thread_local sad_concurrent_queue_write_desciptor_thread_local_alloc(
	sad_pointer_queue_allocator *allocator, fckc_u64 thread_id, fckc_size_t capacity)
{
	sad_concurrent_queue_write_desciptor_thread_local storage;
	storage.thread_id = thread_id;
	storage.tail = sad_concurrent_queue_write_desciptor_block_alloc(allocator, capacity);
	storage.free = NULL;
	storage.current = storage.tail;
	return storage;
}

void sad_concurrent_queue_write_desciptor_thread_local_free(sad_pointer_queue_allocator *allocator,
                                                            sad_concurrent_queue_write_desciptor_thread_local *storage)
{
	sad_concurrent_queue_write_desciptor_block_free(allocator, storage->tail);
}

sad_pointer_queue_write_descriptor *sad_concurrent_queue_write_desciptor_thread_local_consume(
	sad_pointer_queue_allocator *allocator, sad_concurrent_queue_write_desciptor_thread_local *storage)
{

	if (storage->current->used >= storage->current->capacity)
	{

		sad_concurrent_queue_write_desciptor_block *free =
			(sad_concurrent_queue_write_desciptor_block *)sad_pointer_load((void **)&storage->free);
		if (free != NULL)
		{
			sad_pointer_store((void **)&storage->free, free->next_free);
			storage->current->unused = 0;
			storage->current->used = 0;
		}
		else
		{
			const fckc_size_t next_block_size = storage->tail->used << 2;
			sad_concurrent_queue_write_desciptor_block *next_block =
				sad_concurrent_queue_write_desciptor_block_alloc(allocator, next_block_size);

			next_block->prev = storage->tail;
			storage->tail = next_block;
			storage->current = storage->tail;
		}
	}

	fckc_size_t at = storage->current->used;
	storage->current->used = storage->current->used + 1;
	sad_pointer_queue_write_descriptor *result = storage->current->buffer + at;
	result->value = NULL;
	result->block = storage->current;
	return storage->current->buffer + at;
}

void sad_concurrent_queue_write_desciptor_thread_local_recycle(sad_concurrent_queue_write_desciptor_thread_local *storage,
                                                               sad_concurrent_queue_write_desciptor_block *block)
{
	void *free = sad_pointer_load((void **)&storage->free);
	block->next_free = (sad_concurrent_queue_write_desciptor_block *)free;
	sad_pointer_store((void **)&storage->free, block);
}

sad_concurrent_queue_write_desciptor_allocator sad_concurrent_queue_write_desciptor_allocator_alloc(sad_pointer_queue_allocator *allocator)
{
	sad_concurrent_queue_write_desciptor_allocator wd_allocator;
	memset(&wd_allocator, 0, sizeof(wd_allocator));

	for (fckc_size_t index = 0; index < 32; index++)
	{
		wd_allocator.storages[index].thread_id = ~0; // very unlikely this one is getting used...
		wd_allocator.storages[index].tail = NULL;
	}
	return wd_allocator;
}

sad_pointer_queue_write_descriptor *sad_concurrent_queue_write_desciptor_allocator_get(
	sad_pointer_queue_allocator *allocator, sad_concurrent_queue_write_desciptor_allocator *wd_allocator)
{
	const fckc_u64 thread_id = sad_thread_current_id(); // Maybe hash? i do not know
	const fckc_u64 index = thread_id % 32;              // Might make sense to fix all 32

	for (fckc_size_t i = index; i < 32; i++)
	{
		sad_concurrent_queue_write_desciptor_thread_local *storage = &wd_allocator->storages[i % 32];
		if (storage->tail == NULL)
		{
			// Create
			*storage = sad_concurrent_queue_write_desciptor_thread_local_alloc(allocator, thread_id, 16);
		}

		if (storage->thread_id == thread_id)
		{
			sad_pointer_queue_write_descriptor *result = sad_concurrent_queue_write_desciptor_thread_local_consume(allocator, storage);
			result->owner = thread_id;
			return result;
		}
	}
	// I hope that never happens :(
	return NULL;
}

void sad_concurrent_queue_write_desciptor_allocator_recycle(sad_concurrent_queue_write_desciptor_allocator *wd_allocator,
                                                            sad_concurrent_queue_write_desciptor_block *block, fckc_u64 thread_id)
{
	block->unused = block->unused + 1;
	if (block->used == block->capacity && block->unused == block->capacity)
	{
		const fckc_u64 index = thread_id % 32;
		for (fckc_size_t i = index; i < 32; i++)
		{
			sad_concurrent_queue_write_desciptor_thread_local *storage = &wd_allocator->storages[i % 32];
			if (storage->tail == NULL)
			{
				// CRASH... Actually
				break;
			}

			if (storage->thread_id == thread_id)
			{
				sad_concurrent_queue_write_desciptor_thread_local_recycle(storage, block);
			}
		}
	}
}

void sad_concurrent_queue_write_desciptor_allocator_free(sad_pointer_queue_allocator *allocator,
                                                         sad_concurrent_queue_write_desciptor_allocator *wd_allocator)
{
	for (fckc_size_t index = 0; index < 32; index++)
	{
		sad_concurrent_queue_write_desciptor_thread_local_free(allocator, &wd_allocator->storages[index]);
		wd_allocator->storages[index].tail = NULL;
	}
	allocator = NULL;
}

// typedef struct sad_write_descriptors
//{
//	sad_pointer_queue_allocator *allocator;
//	fckc_size_t capacity;
//	sad_pointer_queue_write_descriptor *buffer;
//	sad_atomic_u32 *stack;
//	sad_atomic_u32 count;
// } sad_write_descriptors;
//
// static sad_write_descriptors sad_write_descriptors_alloc(sad_pointer_queue_allocator *allocator, fckc_size_t capacity)
//{
//	sad_write_descriptors descs;
//	descs.allocator = allocator;
//	descs.buffer = allocator->realloc(allocator->context, NULL, capacity * sizeof(*descs.buffer));
//	descs.stack = allocator->realloc(allocator->context, NULL, capacity * sizeof(*descs.stack));
//
//	for (fckc_size_t index = 0; index < capacity; index++)
//	{
//		descs.buffer[index].value = NULL;
//		descs.buffer[index].next = NULL;
//		descs.buffer[index].pending.value = 0;
//	}
//	// Initialise shared stack of free write descriptors
//	for (fckc_size_t index = 0; index < capacity; index++)
//	{
//		descs.stack[index].value = index;
//	}
//	descs.capacity = capacity;
//	descs.count.value = capacity;
//	return descs;
// }
//
// static void sad_write_descriptors_free(sad_write_descriptors *descs)
//{
//	sad_pointer_queue_allocator *allocator = descs->allocator;
//	descs->buffer = allocator->realloc(allocator->context, descs->buffer, 0);
//	descs->stack = allocator->realloc(allocator->context, descs->stack, 0);
//	descs->count.value = 0;
//	descs->capacity = 0;
// }

typedef struct sad_pointer_queue_node
{
	void *value;
	struct sad_pointer_queue_node *next;
} sad_pointer_queue_node;

typedef struct sad_pointer_queue
{
	// sad_write_descriptors write_descriptors;
	sad_concurrent_queue_write_desciptor_allocator wd_allocator;
	sad_pointer_queue_allocator *allocator;

	sad_pointer_queue_node *nodes;

	sad_pointer_queue_node *tail;
	sad_pointer_queue_node *head;

	sad_pointer_queue_write_descriptor *wd;
	fckc_u32 capacity;
	sad_atomic_u32 executing;

} sad_pointer_queue;

sad_pointer_queue *sad_pointer_queue_alloc(fckc_u32 capacity, sad_pointer_queue_allocator *allocator)
{
	capacity = capacity + 1;
	const fckc_size_t total_element_size = (sizeof(sad_pointer_queue_node) * capacity);
	const fckc_size_t total_size = sizeof(sad_pointer_queue) + total_element_size;

	fckc_u8 *memory = (fckc_u8 *)allocator->realloc(allocator->context, NULL, total_size);
	sad_pointer_queue *pointer_queue = (sad_pointer_queue *)memory;
	pointer_queue->nodes = (sad_pointer_queue_node *)(memory + sizeof(sad_pointer_queue));
	pointer_queue->capacity = capacity;

	sad_pointer_queue_node *node = &pointer_queue->nodes[0];
	node->next = node;
	node->value = NULL;

	pointer_queue->tail = &pointer_queue->nodes[0];
	pointer_queue->head = &pointer_queue->nodes[0]; // TODO: Fix up root
	pointer_queue->allocator = allocator;
	pointer_queue->wd = NULL;
	pointer_queue->executing.value = 0;

	pointer_queue->wd_allocator = sad_concurrent_queue_write_desciptor_allocator_alloc(allocator);
	// pointer_queue->write_descriptors = sad_write_descriptors_alloc(allocator, 1024);
	return pointer_queue;
}

void sad_pointer_queue_free(sad_pointer_queue *queue)
{
	queue->allocator->realloc(queue->allocator->context, queue, 0);
}

sad_pointer_queue *sad_pointer_queue_enqueue(sad_pointer_queue *queue, void *value)
{
	// sad_write_descriptors *write_descriptors = &queue->write_descriptors;

	const fckc_u16 invalid_index = (fckc_u16)~0;
	// void **wd_ptr = (void **)&queue->wd;

	{ // Push to the smol stack
		sad_pointer_queue_write_descriptor *wd = sad_concurrent_queue_write_desciptor_allocator_get(queue->allocator, &queue->wd_allocator);
		wd->value = value;
		wd->owner = sad_thread_current_id();
		while (1)
		{
			wd->next = (sad_pointer_queue_write_descriptor *)sad_pointer_load((void **)&queue->wd); // We hold NULL
			if (sad_pointer_cas((void **)&queue->wd, wd->next, wd) == wd->next)
			{
				break;
			}
		}
	}

	// The following control block is locked when a thread entered
	// This one thread steals the whole stack of write descriptors
	// and then places it into the actual queue, if another thread
	// is already doing the work, we just skip and we know our item
	// will see the queue eventually
	while (sad_u32_cas(&queue->executing, 0, 1) == 0)
	{
		// Pop from the small stack and do some work! We are not doing a check here cause we KNOW pd_ptr exists
		sad_pointer_queue_write_descriptor *current = (sad_pointer_queue_write_descriptor *)sad_pointer_store((void **)&queue->wd, NULL);
		while (current != NULL)
		{
			sad_pointer_queue_write_descriptor *next = current->next;
			void *value = current->value;
			//sad_concurrent_queue_write_desciptor_allocator_recycle(&queue->wd_allocator, current->block, current->owner);
			current = next;

			// Nobody is touching the tail for writing... That should hold up
			sad_pointer_queue_node *old_tail = (sad_pointer_queue_node *)sad_pointer_load((void **)&queue->tail);

			fckc_size_t distance = (1 + (fckc_size_t)(old_tail - queue->nodes)) % queue->capacity;
			sad_pointer_queue_node *new_node = queue->nodes + distance;

			// Make these transactions safe
			// - We can prepare new_node safely
			// - We can update the tail safely since dequeue never looks at tail and the work on the queue is linearlised
			// - Updating old tail's next has to be taken care of well

			// New node points to itself indicating invalid state
			// We do that so the tail remembers where to append items to and for the tail to know when to stop
			sad_pointer_store((void **)&new_node->next, new_node);
			// Old tail is holding the value, ready to get dequeues. We can peek on its value to do a second check for validity
			sad_pointer_store((void **)&old_tail->value, value);
			sad_pointer_store((void **)&queue->tail, new_node);

			// Setting this clears the old tail node from pointing to itself to pointing to the next one
			// Essentially making it available to the consumers (dequeue)
			// We only need to fence off the next pointer! Until we present it, we know we have exlusive access
			sad_pointer_store((void **)&old_tail->next, new_node);

		}

		// Finally done, otherwise, continue with more work.
		// This could technically starve us out like crazy, but so be it...
		sad_u32_store(&queue->executing, 0);
		if (sad_pointer_cas((void **)&queue->wd, NULL, NULL) == NULL)
		{
			break;
		}
		// If the condition above is not met, we know a new wd_ptr appeared.
		// It can make sense to flicker executing off, by just doing sad_u32_store(&queue->executing, 0);
		// And then another thread might take over... Or not
	}

	return queue;
}

void sad_pointer_queue_print(sad_pointer_queue *queue)
{
	for (fckc_size_t index = 0; index < 32; index++)
	{
		fckc_size_t total = 0;
		fckc_size_t elements = 0;
		sad_concurrent_queue_write_desciptor_block *current = queue->wd_allocator.storages[index].tail;
		while (current != NULL)
		{
			total = total + 1;
			elements = elements + current->capacity;
			current = current->prev;
		}

		printf("(%llu) - Blocks: %llu  - Elements %llu\n", index, total, elements);
	}
}

int sad_pointer_queue_dequeue(sad_pointer_queue *queue, void **payload)
{
	// A node pointing to itself indicates it is NOT valid - Might be as valid to just set it to NULL...
	sad_pointer_queue_node *head;
	sad_pointer_queue_node *next;
	do
	{
		head = (sad_pointer_queue_node *)sad_pointer_load((void **)&queue->head);
		// We only need to fence off the next pointer!
		next = (sad_pointer_queue_node *)sad_pointer_load((void **)&head->next);
		if (next == head)
		{
			return 0;
		}
	} while (sad_pointer_cas((void **)&queue->head, head, next) != head);

	*payload = head->value;
	return 1;
}