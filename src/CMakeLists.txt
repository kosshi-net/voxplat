# Include files/* to object files.o with GNU Linker
# The working directory needs to be set for object names to stay consistant
add_custom_command(
	OUTPUT resources.o
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/resources
	COMMAND ld -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/resources.o *
)


add_subdirectory( 
	cpp
) # NOISE

add_library( ctx 
	ctx.c	
	glad.c
	ctx/input.c
)

add_library( gfx
	gfx.c
	gfx/text.c
	gfx/rect.c
	gfx/sky.c
	gfx/aabb.c
	
	gfx/crosshair.c
	gfx/shell.c

	gfx/fcull.c
	gfx/vsplat.c
	gfx/vmesh.c
	gfx/camera.c

	gfx/ppm.c
) 

add_library( res
	res.c
	resources.o
)

add_library( ecs
	entity.c
)

add_executable( game
	main.c	
	cfg.c	
	game.c

	threadpool.c

	chunkset.c
	chunkset/edit.c
	chunkset/mesher.c

	chunkset/rle.c
	chunkset/gen.c

	event.c
	mem.c	
	res.c

	shell.c	
)
set_property(TARGET game PROPERTY C_STANDARD 99)

target_compile_options( game PUBLIC
	"-O3"
	"-march=native"
	"-ftree-vectorize"
	"-Wformat-truncation=0"
	"-Wall"
	"-static"
	"${OpenMP_CXX_FLAGS}"
)

target_link_libraries( game
	gfx 
	ctx	
	res	
	noise	
	ecs	
	
#	"-static"
#	"-static-libgcc"

	"${OpenMP_CXX_FLAGS}"	
	OpenMP::OpenMP_CXX

	glfw
	freetype
)
