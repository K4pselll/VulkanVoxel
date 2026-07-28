# VulkanVoxel

A Minecraft-style voxel engine built from scratch with C++ and Vulkan.

## Features

- 64x64 world with blocks, slabs, stairs, doors, and glass
- Block placing/breaking, 9 block types
- Build modes: block, slab, stair (cycle with C key)
- Crouch (hold Shift), fly (double-jump Space)
- Save/load worlds (Esc to save in-game)
- Main menu with save management

## Controls

| Key | Action |
|-----|--------|
| WASD | Move |
| Mouse | Look |
| Space | Jump / double-jump to fly |
| Shift | Crouch / fly down |
| C | Cycle build mode |
| Left Click | Break block |
| Right Click | Place block |
| 1-9 | Select block type |
| Esc | Open menu / Save & quit |

## Build

### Windows (MSYS2 UCRT64)

1. Install [MSYS2](https://www.msys2.org/), open **UCRT64** terminal
2. Install build tools:
   ```
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja git
   ```
3. Install [Vulkan SDK](https://vulkan.lunarg.com/) (Windows installer)
4. Restart UCRT64 terminal (so it picks up VULKAN_SDK env var)
5. Build:
   ```
   git clone https://github.com/K4pselll/VulkanVoxel.git
   cd VulkanVoxel
   cmake -B build -G Ninja
   cmake --build build
   cd build
   ./voxel.exe
   ```

### Linux

```
sudo apt install build-essential cmake ninja-build git libvulkan-dev glslc-tools
git clone https://github.com/K4pselll/VulkanVoxel.git
cd VulkanVoxel
cmake -B build -G Ninja
cmake --build build
cd build
./voxel
```

### Docker (Linux)

```
git clone https://github.com/K4pselll/VulkanVoxel.git
cd VulkanVoxel
docker build -t vulkanvoxel .
docker run --rm -it --device=/dev/dri:/dev/dri -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix vulkanvoxel
```

> Note: Docker requires `--device` and X11 passthrough for GPU access. On Wayland, use `--security-opt seccomp=unconfined` and appropriate Wayland socket mounts.

### Installer (Windows)

After building with MSYS2, create a distributable installer:

```
pacman -S mingw-w64-ucrt-x86_64-nsis
makensis installer.nsi
```

Output: `VulkanVoxel-2.0-Setup.exe` — a single-file installer that puts the game in Program Files, creates Start Menu + Desktop shortcuts, and supports uninstall via Windows Apps & Features.

## Dependencies

- Vulkan SDK (1.3+) — provides headers, Vulkan loader, and `glslc` shader compiler
- GLFW 3.4 — fetched automatically by CMake
- GLM 1.0.1 — fetched automatically by CMake
- C++17 compiler (g++ 11+, MSVC 2019+)
- CMake 3.20+
- Ninja (recommended) or Make

All other dependencies (shaders, models) are included in the repository.
