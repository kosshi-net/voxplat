# Voxplat

A hybrid voxel rendering engine, written with C and OpenGL. The goal of this
project was to make a engine capable of rendering a large amount of
voxels on mid-tier graphics cards.

8192x512x8192 map with over 120 000 trees, rendered at 130fps on Radeon RX 480. 128^3 chunks, 4.5 GB peak ram usage, FPS limited by draw calls.
![Pretty picture](https://kosshi.net/u/jzn6y.png)
![Pretty picture](https://kosshi.net/u/jzldm.png)

## Rendering method

This is achieved with OpenGL splatting, basically rendering quads and ray-aabb
testing in a fragment shader. This reduces the vertex count to just 1 per voxel, 
but with a somewhat slow fragment shader. This is a real probelm with nearby
voxels, so hybrid rendering is used: meshes for nearby, splatting farther away.

Engine implements SVO-like level of detail, with first level starting at 1024, 
voxels away.

### Relevant
This [Nvidia paper](http://www.jcgt.org/published/0007/03/04/) has a different, 
more refined version of the splatting part.

### Work in progress!
This is also my first large C project, there is a lot of code and a lot of it 
is ugly and temporary. Especially the mesher. Don't look at the mesher.

### Launch options
List of some launch options and their default values:

``--heap 512M``

How much memory to allocate. 
You must adjust this for larger worlds.

``--chunk_size 64``

Root of the chunks. You must use power of twos!

``--world_size 2K 256 2K``

Size of the world. Only power of twos!

``--opengl-compat``

Use OpenGL compatability mode. Might help if you have trouble running the engine.


## Engine features
- OpenGL splatting capable of drawing large scenes at high framerates
- Standard meshed rendering for nearby voxels for additional effects and speed
- Fast culler and mesher (a lot of it is still very WIP)
	- 64^3 in 1-4 milliseconds singlethreaded on a Ryzen 1600 @ 3.8GHz
- Subtle level of detail (starts at 1024 voxels away)
- Block placement and removal
- Completetly fake shadows and ambient occlusion
- Custom world generator for pretty screenshots
- No hardcoded world or chunk size
- RLE compression
- Simple custom malloc implementation
- Ingame console (F12 to toggle)
- Texture support, but its hard disabled for looks. I like solidcolor voxels :)
- Builds on Linux and Windows (with MinGW)


Some pictures:
![Pretty picture](https://i.imgur.com/ytUnnra.jpg)
![Pretty picture](https://i.imgur.com/BmAt7jr.jpg)
![Pretty picture](https://i.imgur.com/Gw4Vdu5.jpg)
More pretty pictures and history at https://imgur.com/a/6zwciLy

## Download
(WIP)

## Building
CMAKE Required.

For libraries, either consult CMakeLists.txt and download them all one by one,
or you can download the following zip file with a copy of the libraries used 
(make sure contents are in ``./ext/``).
https://kosshi.net/u/voxplat-0-7-libraries.zip

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
- OpenMP
- FastNoise
- FreeType

## License
No license - I wish to keep my rights to it for now.