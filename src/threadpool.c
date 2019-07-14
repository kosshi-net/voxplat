#include "threadpool.h"
#include "mem.h"
#include "event.h"

#include <pthread.h>
#include <string.h>

#include <unistd.h>

#define THREAD_COUNT 6
#define MAX_TASK_COUNT 256

pthread_t worker_pool[THREAD_COUNT];

struct Task {
	void (*fn)(void*);
	void *arg;
};

struct Task		*task_queue;
uint16_t 		 task_queue_head = 0;
uint16_t 		 task_queue_tail = 0;
pthread_mutex_t  task_mutex;

pthread_mutex_t  signal_mutex;
pthread_cond_t 	 signal;

void* worker_loop( void*a ){
	struct Task task;
	while( 1 ) {



		pthread_mutex_lock(&task_mutex);

		if (task_queue_tail == task_queue_head){
			pthread_mutex_unlock(&task_mutex);

			pthread_mutex_lock(&signal_mutex);
			pthread_cond_wait(&signal, &signal_mutex);
			pthread_mutex_unlock(&signal_mutex);

			continue;
		}

		memcpy( &task,
			&task_queue[task_queue_head], 
			sizeof(struct Task)
		);
		task_queue_head = (task_queue_head+1) % MAX_TASK_COUNT;
		pthread_mutex_unlock(&task_mutex);

		task.fn( task.arg );
		if( task.arg ) task.arg = mem_free(task.arg);
	}
	return NULL;
}


int	threadpool_init(void){
	pthread_mutex_init(&task_mutex, NULL);
	pthread_mutex_init(&signal_mutex, NULL);
	task_queue = mem_calloc( 
		sizeof(struct Task) * MAX_TASK_COUNT 
	);
	for (int i = 0; i < THREAD_COUNT; ++i)
		pthread_create( &worker_pool[i], NULL, worker_loop, NULL);
	return 0;
}

void threadpool_task( void(*fn)(void*), void*arg ){
	task_queue[ task_queue_tail ].fn = fn;
	task_queue[ task_queue_tail ].arg = arg;
	task_queue_tail = (task_queue_tail+1) % MAX_TASK_COUNT;

	pthread_mutex_lock(&signal_mutex);
	pthread_cond_signal(&signal);
	pthread_mutex_unlock(&signal_mutex);

}