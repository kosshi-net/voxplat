# Voxplat
## Note: Readme work in progress

A hybrid voxel rendering engine, written with C and OpenGL. The goal of this
project was to a renderer capable of rendering a large amount of
voxels on mid-tier graphics cards.

8192^2 area with over 100 000 trees, rendered at 170fps on AMD RX 480
![Pretty picture](https://i.imgur.com/tnNTgnH.jpg)

## Rendering

This is achieved with OpenGL splatting, basically rendering quads and ray-aabb
testing in a fragment shader. This reduces the vertex count to just 1 per voxel, 
with the cost of a slow fragment shader. This is a real probelm with nearby
voxels, so hybrid rendering is used: meshes for nearby, splatting farther away.

### Relevant
This [nvidia paper](http://www.jcgt.org/published/0007/03/04/) has a different, 
more refined version of the splatting part.

Some pictures:
![Pretty picture](https://i.imgur.com/ytUnnra.jpg)
![Pretty picture](https://i.imgur.com/BmAt7jr.jpg)
![Pretty picture](https://i.imgur.com/Gw4Vdu5.jpg)
More pretty pictures and history at https://imgur.com/a/6zwciLy

## Engine features
- OpenGL splatting archieving high drawdistances and high framerates
- Standard meshed rendering for nearby voxels for additional effects and speed
- Fast culler and mesher (a lot of it is still very WIP)
	- 64^3 in 1-4 milliseconds singlethreaded on a Ryzen 1600 @ 3.8GHz
- Barely noticeable level of detail (starts at 1024 voxels away)
- Block placement and removal
- Completetly fake shadows and ambient occlusion
- Custom world generator for pretty screenshots
- RLE compression
- Ingame console
- Builds on Linux and Windows (with MinGW)


## Building
- Requires CMAKE
- For now consult CMakeLists.txt for placement of libraries

### Linux
```
./clear.sh
./build.sh
```

### Windows (MINGW64)
```
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

## Libraries used
- glfw
- glad (included)
- cglm
- stb/std_image.h
- OpenMP
- FastNoise
- FreeType
