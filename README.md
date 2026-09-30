# VoxelEngine 🎮

A Minecraft-inspired voxel engine built from scratch with **C++20** and **OpenGL 4.3**.

![Status](https://img.shields.io/badge/status-in_development-yellow)
![Language](https://img.shields.io/badge/language-C%2B%2B20-blue)
![Graphics](https://img.shields.io/badge/graphics-OpenGL%204.3-green)

## Features

- 🌍 **Procedural terrain generation** using multi-octave simplex noise (FastNoiseLite)
- ⛏️ **Block types**: Grass, Dirt, Stone, Sand, Water, Snow, Bedrock, Wood, Leaves
- 🗺️ **Chunk-based world** with dynamic loading/unloading around the player
- 🎨 **Procedural block coloring** with per-type shading (no texture atlas needed)
- 💡 **Sun + hemisphere lighting** with ambient sky tinting
- 🌫️ **Distance fog** for smooth horizon blending
- 🕳️ **Cave generation** using 3D noise
- 🏔️ **Biome-like height variation** (snow peaks, sand beaches, green hills)
- 🎥 **FPS camera** with smooth mouse look and WASD movement
- 🔄 **Shader hot-reload** (press R or F5)
- 🔲 **Wireframe toggle** (F1)
- 📊 **Real-time stats** in window title (FPS, chunk count, triangle count, position)

## Controls

| Key | Action |
|-----|--------|
| `WASD` | Move |
| `Mouse` | Look around |
| `Space` | Fly up |
| `Shift` | Fly down |
| `Ctrl` | Sprint |
| `F1` | Toggle wireframe |
| `R` / `F5` | Hot-reload shaders |
| `Escape` | Toggle cursor capture |
| `Q` | Quit |

## Building

### Prerequisites

- **CMake** 3.20+
- **C++20 compiler**: MSVC (Visual Studio Build Tools), GCC 10+, or Clang 12+
- **OpenGL 4.3** compatible GPU

### Build Commands

```bash
# Configure
cmake -S . -B build

# Build (Debug)
cmake --build build --config Debug

# Build (Release)
cmake --build build --config Release

# Run
./build/Debug/VoxelEngine    # Windows
./build/VoxelEngine           # Linux/macOS
```

### Dependencies (Auto-fetched by CMake)

All dependencies are downloaded automatically via CMake's `FetchContent`:

| Library | Purpose |
|---------|---------|
| [GLFW 3.4](https://github.com/glfw/glfw) | Windowing & input |
| [GLM 1.0.1](https://github.com/g-truc/glm) | Math library |
| [stb](https://github.com/nothings/stb) | Image loading (textures) |
| [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) | Terrain noise generation |

## Project Structure

```
game_engine/
├── CMakeLists.txt           # Build configuration
├── include/                 # Header files
│   ├── glad/glad.h          # OpenGL loader
│   ├── window.h             # GLFW window wrapper
│   ├── shader.h             # GLSL shader manager
│   ├── camera.h             # FPS camera
│   ├── chunk.h              # Voxel chunk (16x128x16)
│   ├── world.h              # Chunk manager
│   ├── renderer.h           # Render pipeline
│   └── input.h              # Input handler
├── src/                     # Source files
│   ├── main.cpp             # Entry point & game loop
│   ├── glad.c               # OpenGL function loader
│   ├── window.cpp
│   ├── shader.cpp
│   ├── camera.cpp
│   ├── chunk.cpp
│   ├── world.cpp
│   ├── renderer.cpp
│   └── input.cpp
├── shaders/                 # GLSL shaders (hot-reloadable)
│   ├── chunk.vert
│   └── chunk.frag
└── .vscode/                 # VS Code debug/build config
    ├── launch.json
    └── tasks.json
```

## Architecture

```
main.cpp (game loop)
  ├── Window (GLFW + OpenGL context)
  ├── Input (keyboard + mouse → Camera)
  ├── Camera (view/projection matrices)
  ├── World (chunk loading/unloading)
  │   └── Chunk[] (terrain gen → mesh → GPU)
  └── Renderer (shaders, state, fog)
```

## Roadmap

- [ ] Texture atlas support
- [ ] Tree generation
- [ ] Block breaking/placing
- [ ] Greedy meshing optimization
- [ ] Multithreaded chunk generation
- [ ] Ambient occlusion
- [ ] Shadow mapping
- [ ] Water reflections
- [ ] Day/night cycle
- [ ] Physics & collision

## License

MIT
