#define __FILENAME__ "ctx.c"

#include <config.h>

#include "ctx.h"
#include "event.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>

// CTX
// Abstraction layer for glfw and other "os" stuff


static void error_callback(int error, const char* description){
	printf( "ERROR! %s", description  );
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
//	logf_info("Window resized to %i %i", width, height);
	glViewport(0, 0, width, height);
}

GLFWwindow* window;

int ctx_init()
{	
	log_info( "Init"  );
	if (!glfwInit())
	{
		log_error("GLFW Failed!");
		return 1;		
	}
	glfwSetErrorCallback(error_callback);

	// OpenGL 3.3 Core
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
	//glfwWindowHint(GLFW_SAMPLES, MSAA_SAMPLES);
	//
	glfwWindowHint( GLFW_DOUBLEBUFFER, GL_FALSE );


    window = glfwCreateWindow(640, 480, PROJECT_NAME, NULL, NULL);
	glfwSwapInterval(0);

    if (!window) {
        glfwTerminate();
        return 2;
    }

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwMakeContextCurrent(window);

	if(!gladLoadGL()) 
	{
		log_error("GLAD Failed!");
		return 2;
	}
	const unsigned char*renderer = glGetString(GL_RENDERER); 
	const unsigned char*version  = glGetString(GL_VERSION); 
	logf_info("%s", renderer);
	logf_info("OpenGL %s", version);
	   	logf_info("CTX GLAD %d.%d", GLVersion.major, GLVersion.minor);

	log_info( "Init ok"  );
	return 0;
}

void ctx_terminate() {
	glfwTerminate();
}

int ctx_should_close(){
	return glfwWindowShouldClose(window);
}

GLFWwindow* ctx_get_window() {
	return window;
}

void ctx_swap_buffers() {
	//glfwSwapBuffers(window);
	glFlush();
}

void ctx_poll_events() {
	glfwPollEvents();
}

void ctx_get_window_size(int*width, int*height){
	glfwGetWindowSize(window, width, height);
}

void ctx_set_window_title(char* title){
	glfwSetWindowTitle(window, title);
}

double ctx_time() {
	return glfwGetTime();
}





