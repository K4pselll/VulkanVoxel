# VulkanVoxel

A Minecraft-style voxel engine built from scratch with C++ and Vulkan.

## Features

- Flat 64x64 world with grass, dirt, and stone layers
- Block placing and breaking (left/right click)
- 7 block types: Grass, Dirt, Stone, Sand, Wood, Water, Brick
- Block selection hotbar (keys 1-7)
- Block highlight wireframe
- Walking with gravity + double-jump to toggle fly mode
- WASD movement, mouse look, crosshair HUD
- Face culling, directional lighting, fog

## Controls

| Key | Action |
|-----|--------|
| WASD | Move |
| Mouse | Look |
| Space | Jump / double-jump to fly |
| Shift | Fly down / land to walk |
| Left Click | Break block |
| Right Click | Place block |
| 1-7 | Select block type |
| Esc | Quit |

## Build

Requires [Vulkan SDK](https://vulkan.lunarg.com/) and [CMake](https://cmake.org/) 3.20+.

```bash
cmake -B build -G Ninja
cmake --build build
cd build
./voxel.exe
```

GLM and GLFW are fetched automatically via CMake FetchContent.
