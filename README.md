# Voxplat
A hybrid voxel rendering engine
## Note: Readme work in progress

Mainly uses splatting, uses regular mesh for nearby voxels for better 
performance and additional effects

https://kosshi.net/experiments/voxplat.html

![Pretty picture](https://kosshi.net/u/efexh.png)

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
- FastNoise
- FreeType
- stb/std_image.h
