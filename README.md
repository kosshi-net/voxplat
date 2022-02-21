# Voxplat

A hybrid voxel rendering engine, written with C and OpenGL. The goal of this
project is to make a voxel engine capable of rendering huge fully destructible 
scenes on reasonable desktop hardware. And maybe eventually a game.



https://user-images.githubusercontent.com/19539479/154972268-bdbe3cf9-b18a-4307-824b-a1baf84e7d46.mp4

Higher quality version: https://kosshi.net/img/voxplat16k.mp4

Build in video uses unfinished lighting which has some popin and other minor bugs.

The map is 16384 x 512 x 16384 voxels, all stored in memory (128GB if uncompressed). 

GPU used is an AMD RX480 4GB. 

## Rendering method
This is achieved with OpenGL splatting, basically rendering quads and ray-aabb
testing in a fragment shader. This reduces the vertex count to just 1 per voxel, 
but with a somewhat slow fragment shader. This becomes a real probelm with nearby
voxels, so hybrid rendering is used: meshes for nearby, splatting farther away.

All voxels are stored in flat arrays, rle compressed when inactive. 
Level of detail is implemented by "mipmapping" visible voxels of a 
chunk and grouping geometry by lod level to an octree-like structure to reduce 
draw calls. LOD kicks in at 1024 voxels away, far enough that single voxels are 
almost a single pixel in size at 1080p.

Each wireframe cube is a draw call:
![Pretty picture](https://kosshi.net/img/tcCm.png)

### Textures?
There was support a while back, see the album linked below. The splatter 
can do them just fine. But the meshing and rendering are already complex and 
messy enough to develop without textures, dropped support until those system
are less temp-code. I also just like solidcolor voxels :)

### Relevant
Splatting method was inspired by this [paper](http://www.jcgt.org/published/0007/03/04/)

## Eternally Work in progress!
This is my first large C project, there is a lot of code and most of it needs refactoring. 

This project has been on a hiatus since my military service, and I'm not sure if I will continue it.

## Launch options
List of some launch options and their default values:

``--heap 1024M`` How much memory to allocate. You must adjust this for larger 
worlds.

``--chunk_size 64``Root of the chunks. You must use power of twos!

``--world_size 2K 256 2K`` Size of the world. Only power of twos!

``--opengl-compat`` Use OpenGL compatability mode. Might help with buggier drivers.


## Engine features
- OpenGL splatting capable of drawing large scenes at high framerates
- Multithreaded, world editing and meshing is highly scalable and framerates always stable
- Standard meshed rendering for nearby voxels for additional effects and speed
- Fast culler and mesher (a lot of it is still very WIP)
	- 64^3 in 1-4 milliseconds singlethreaded on a Ryzen 1600
- Subtle level of detail (starts at 1024 voxels away)
- Block placement and removal
- Completetly fake shadows and ambient occlusion
- Custom world generator for pretty screenshots
- No hardcoded world or chunk size
- RLE compression
- Custom general purpose memory allocator
- Ingame console (F12 to toggle)
- Builds on Linux and Windows (with MinGW)

## Limitations
- 16k^3 largest possible volume due to 16 bit-isms in mostly rendering
- Maximum 256 chunk root (see rle.c)

Some pictures:

![Pretty picture](img/1.png?raw=true)
![Pretty picture](img/3.png?raw=true)
![Pretty picture](https://i.imgur.com/ytUnnra.jpg)
![Pretty picture](https://i.imgur.com/Gw4Vdu5.jpg)
More pretty pictures and history at https://imgur.com/a/6zwciLy

## Building
CMAKE Required.
Libraries not included.

### Linux
```
mkdir build; cd build;
cmake ..;
make;
```

``clear.sh`` and ``build.sh`` are for development use.

### Windows (MINGW64)
```
mkdir build; cd build;
cmake .. -G "MinGW Makefiles";
mingw32-make;
```

## Libraries used
- glfw
- glad (included)
- cglm
- stb/std_image.h
- OpenMP and pthreads (and other POSIX apis)
- FastNoise
- FreeType

## License
No license - I wish to keep my rights to it for now.
