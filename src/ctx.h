#ifndef CTX_H
#define	CTX_H 1

#ifdef CTX_H_INCLUDE_GLFW
#include <GLFW/glfw3.h>
GLFWwindow* ctx_get_window(void);
#endif

int ctx_init(void);

int ctx_should_close(void);

void ctx_terminate(void);


void ctx_poll_events(void);
void ctx_swap_buffers(void);

void ctx_get_window_size(int*, int*);
void ctx_set_window_title(char*);

double ctx_time(void);


#endif
