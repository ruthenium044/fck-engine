#include <kll.h>
#include <kll_heap.h>
#include <kll_malloc.h>
#include <sad_atomic.h>
#include <sad_pointer_queue.h>
#include <sad_thread.h>

#include <stdio.h>

void *sad_allocator_implementation(void *context, void *ptr, size_t size)
{
	return kll_realloc(kll_heap, ptr, size);
}

#define TEST_QUEUE_SIZE (1 << 10)
#define TEST_THREAD_PRODUCER_COUNT 5
#define TEST_THREAD_CONSUMER_COUNT 5

static sad_pointer_queue *shared_queue = NULL;

typedef struct big_data
{
	fckc_u64 data[8];
} big_data;

static int thread_produce(void *args)
{
	big_data big_data;
	big_data.data[0] = sad_thread_current_id();
	big_data.data[1] = big_data.data[0];
	big_data.data[2] = big_data.data[0];
	big_data.data[3] = big_data.data[0];
	big_data.data[4] = big_data.data[0];
	big_data.data[5] = big_data.data[0];
	big_data.data[6] = big_data.data[0];
	big_data.data[7] = big_data.data[0];

	sad_pointer_queue *queue;
	while (1)
	{
		queue = (sad_pointer_queue *)sad_pointer_load((void **)&shared_queue);
		if (queue != NULL)
		{
			break;
		}
		sad_thread_yield();
	}

	const fckc_size_t count = TEST_QUEUE_SIZE / TEST_THREAD_PRODUCER_COUNT;
	for (fckc_size_t index = 0; index < count; index++)
	{
		sad_pointer_queue_enqueue(queue, &big_data);
	}

	return 0;
}

static int thread_consume(void *args)
{
	sad_pointer_queue *queue;
	while (1)
	{
		queue = (sad_pointer_queue *)sad_pointer_load((void **)&shared_queue);
		if (queue != NULL)
		{
			break;
		}
		sad_thread_yield();
	}

	void *data;
	fckc_size_t count = 0;
	while (1)
	{
		queue = (sad_pointer_queue*)sad_pointer_load((void**)&shared_queue);
		if (queue == NULL)
		{
			sad_thread_yield();
			break;
		}

		while (sad_pointer_queue_dequeue(queue, &data))
		{
			count++;
		}
	}

	*(fckc_size_t *)(args) = count;

	return 0;
}

int main(int argc, char **argv)
{
	sad_pointer_queue_allocator allocator;
	allocator.context = NULL;
	allocator.realloc = sad_allocator_implementation;

	sad_pointer_queue *queue = sad_pointer_queue_alloc(TEST_QUEUE_SIZE, &allocator);
	sad_pointer_store((void **)&shared_queue, queue);

	sad_thread_arguments producer_args[TEST_THREAD_PRODUCER_COUNT];
	sad_thread producers[TEST_THREAD_PRODUCER_COUNT];
	for (fckc_size_t index = 0; index < TEST_THREAD_PRODUCER_COUNT; index++)
	{
		producer_args[index] = sad_thread_arguments_create(thread_produce, NULL);
		producers[index] = sad_thread_open(producer_args + index);
	}

	sad_thread_arguments consumer_args[TEST_THREAD_CONSUMER_COUNT];
	sad_thread consumers[TEST_THREAD_CONSUMER_COUNT];
	fckc_size_t counters[TEST_THREAD_CONSUMER_COUNT];
	for (fckc_size_t index = 0; index < TEST_THREAD_CONSUMER_COUNT; index++)
	{
		consumer_args[index] = sad_thread_arguments_create(thread_consume, counters + index);
		consumers[index] = sad_thread_open(consumer_args + index);
	}

	for (fckc_size_t index = 0; index < TEST_THREAD_PRODUCER_COUNT; index++)
	{
		sad_thread_join(producers[index], SAD_INFINITE);
	}

	sad_pointer_store((void**)&shared_queue, NULL);
	for (fckc_size_t index = 0; index < TEST_THREAD_PRODUCER_COUNT; index++)
	{
		sad_thread_join(consumers[index], SAD_INFINITE);
	}

	void *data;
	fckc_size_t count = 0;
	while (sad_pointer_queue_dequeue(queue, &data))
	{
		count++;
	}

	for (fckc_size_t index = 0; index < TEST_THREAD_CONSUMER_COUNT; index++)
	{
		printf("Thread %llu - Result: %llu\n", index, counters[index]);
		count = count + counters[index];
	}

	printf("Total - Result: %llu Expected: %llu\n", count, TEST_QUEUE_SIZE);
	sad_pointer_queue_print(queue);

	return 0;
}