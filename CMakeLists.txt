cmake_minimum_required (VERSION 3.12)

project ("Voxplat")

set( VERSION_MAJOR 0 )
set( VERSION_MINOR 6 )

### PLATFORM DETECTION ###
	if(APPLE)
		message( FATAL_ERROR "APPLE is not supported" )
	endif()
	if(WIN32)
		if(NOT MINGW)
			message( FATAL_ERROR "Only MinGW is supported on Windows! Use cmake .. -G \"MinGW Makefiles\"")
		endif()
	endif()



### CONFIGURATION ###
	
	# Disable building useless GLFW stuff
	set(GLFW_BUILD_DOCS 	OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS 	OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_EXAMPLES	OFF CACHE BOOL "" FORCE)
	
	# Export stuff for linters
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

	# 3rd party library location
	set( EXTERNAL ext )

	# DEBUG
	set( DEBUG_GFX_TEXT_WRITE_PPM 0 )


	configure_file(config.h.in ${PROJECT_SOURCE_DIR}/include/config.h)


### CLI OPTIONS ###
	option( DEBUG "Compile in debugging mode" )
	
	if(DEBUG)
		set( CMAKE_BUILD_TYPE Debug ) 
		set( CMAKE_BUILD_TYPE RelWithDebInfo )
	endif(DEBUG)

### LIBRARIES ### 

	# GLFW
	add_subdirectory	(${EXTERNAL}/glfw)

	# Freetype
	add_subdirectory	(${EXTERNAL}/freetype)
	include_directories	(${EXTERNAL}/freetype/include)

	# CGLM (header-only)
	include_directories	(${EXTERNAL}/cglm/include)

	# stb/stb_image.h (header-only)
	# Initalized in gfx
	include_directories	(${EXTERNAL}/stb)

	# OPENMP
	find_package(OpenMP REQUIRED)
	# find with find /usr -name omp.h
	# include_directories(/usr/lib/gcc/x86_64-pc-linux-gnu/8.2.1/include/)


	include_directories	(include)	
	include_directories	(src)

### BUILD ###

add_subdirectory(src)

