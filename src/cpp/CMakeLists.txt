
set( directory ${PROJECT_SOURCE_DIR}/${EXTERNAL}/FastNoiseSIMD/FastNoiseSIMD  )

add_library( noise

	${PROJECT_SOURCE_DIR}/${EXTERNAL}/FastNoise/FastNoise.cpp

	noise.cpp
)

include_directories(  
	${PROJECT_SOURCE_DIR}/${EXTERNAL}/FastNoise/
)

target_compile_options( noise PRIVATE
	"-O2"
	"-std=c++11"
#	"-march=core-avx2"
)

