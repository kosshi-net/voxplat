# Voxplat
A hybrid voxel rendering engine
## Note: Readme work in progress

Mainly uses splatting, uses regular mesh for nearby voxels for better 
performance and additional effects

![Pretty picture](https://i.imgur.com/ytUnnra.png)
![Pretty picture](https://i.imgur.com/BmAt7jr.png)
![Pretty picture](https://i.imgur.com/Gw4Vdu5.png)
More pretty pictures and history at https://imgur.com/a/6zwciLy

## Engine features
- Hybrid rendering for large drawdistances and high performance
- Custom world generator

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
