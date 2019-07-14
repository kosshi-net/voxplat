#ifndef THREADPOOL_H
#define THREADPOOL_H 1

int	threadpool_init(void);
void threadpool_task( void(*fn)(void*), void*arg );

#endif
