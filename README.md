# Voxplat

A hybrid voxel rendering engine, written with C and OpenGL. The goal of this
project is to make a voxel engine capable of rendering huge fully destructible 
scenes on reasonable desktop hardware. And maybe eventually a game.

8192x512x8192 map with over 120 000 trees, rendered at 1920x1080 130fps on 
Radeon RX 480. Using 128^3 chunks, 4 GB peak ram usage, FPS limited by draw calls.

![Pretty picture](img/1.png?raw=true)
![Pretty picture](img/2.png?raw=true)

## Rendering method
This is achieved with OpenGL splatting, basically rendering quads and ray-aabb
testing in a fragment shader. This reduces the vertex count to just 1 per voxel, 
but with a somewhat slow fragment shader. This is a real probelm with nearby
voxels, so hybrid rendering is used: meshes for nearby, splatting farther away.

No octrees are used, all voxels are stored in flat arrays, rle compressed when 
inactive. Level of detail is implemented by "mipmapping" visible voxels of a 
chunk. LOD kicks in at 1024 voxels away, far enough that single voxels are less
than a single pixel in size at 1080p.

### Textures?
There was support a while back, see the album linked below. The splatter 
can do them just fine. But the meshing and rendering are already complex and 
messy enough to develop without textures, dropped support until those system
are less temp-code. I also just like solidcolor voxels :)

### Relevant
Splatting method was inspired by this [paper](http://www.jcgt.org/published/0007/03/04/)

## Work in progress!
This is also my first large C project, there is a lot of code and a lot of it 
is ugly and temporary. Especially the mesher. Don't look at the mesher.

## Launch options
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
- OpenMP and pthreads
- FastNoise
- FreeType

## License
No license - I wish to keep my rights to it for now.