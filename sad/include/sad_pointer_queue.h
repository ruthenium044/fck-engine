// sad_pointer_queue.h

#ifndef SAD_POINTER_QUEUE_H_INCLUDED
#define SAD_POINTER_QUEUE_H_INCLUDED

#include <fckc_inttypes.h>

typedef void *(sad_pointer_queue_allocator_realloc)(void *context, void *ptr, size_t size);
typedef struct sad_pointer_queue_allocator
{
	sad_pointer_queue_allocator_realloc *realloc;
	void *context;
} sad_pointer_queue_allocator;

typedef struct sad_pointer_queue sad_pointer_queue;

sad_pointer_queue *sad_pointer_queue_alloc(fckc_u32 capacity, sad_pointer_queue_allocator *allocator);
sad_pointer_queue *sad_pointer_queue_enqueue(sad_pointer_queue *queue, void *value);
int sad_pointer_queue_dequeue(sad_pointer_queue *queue, void **payload);

void sad_pointer_queue_print(sad_pointer_queue *queue);

#endif // !SAD_POINTER_QUEUE_H_INCLUDED
