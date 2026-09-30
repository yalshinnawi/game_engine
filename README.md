# VoxelEngine ⛏️

A Minecraft-inspired voxel engine built from scratch with **C++20** and **OpenGL 4.3**, featuring real-time multiplayer networking, survival walking physics, voxel raycasting for block breaking/placing, 3D synchronized avatars, and an interactive in-game pause menu.

![Status](https://img.shields.io/badge/status-active-brightgreen)
![Language](https://img.shields.io/badge/language-C%2B%2B20-blue)
![Graphics](https://img.shields.io/badge/graphics-OpenGL%204.3-green)
![Multiplayer](https://img.shields.io/badge/multiplayer-TCP%20Sockets-orange)

---

## Features

- 🌐 **Real-Time Multiplayer Networking**: Background Winsock TCP server & client architecture syncing player positions, head orientations, and block break/place events at 30 Hz.
- 🧑‍🤝‍🧑 **3D Animated Player Avatars**: Fully rendered voxel character models with rotating head, body, arms, and legs that sync over the network.
- 🏃 **Survival & Creative Modes**:
  - **Survival Mode**: AABB bounding box physics, realistic gravity (`-24 m/s²`), jump detection, and terrain collision resolution.
  - **Creative Flight Mode**: Free noclip flying with smooth vertical movement.
- 🧱 **Block Breaking & Placing**: Fast voxel DDA raycasting with a 5-block reach. Left-click breaks blocks; right-click places blocks (with player body overlap prevention).
- 🎨 **Inventory Material Selection**: Switch placement material (Dirt, Stone, Wood, Sand, Leaves) using keys `1`–`5` or mouse wheel.
- 📜 **Interactive In-Game Pause Menu**: Embedded 8x8 bitmap font system with drop shadows, live button hover states, mode toggling, server hosting/joining, and live network diagnostic banners.
- 🌲 **Procedural Terrain Generation**: Multi-octave Simplex noise (FastNoiseLite) with height variation (peaks, beaches, rolling hills) and 3D cave generation.
- 💡 **Sun + Hemisphere Lighting & Distance Fog**: Directional sunlight, ambient sky tinting, and exponential fog.
- 📦 **Standalone Zero-Dependency Release**: Statically linked MSVC runtime (`/MT`), self-contained in a ready-to-share `.zip` distribution.

---

## Controls

| Key / Input | Action |
|:---|:---|
| `W`, `A`, `S`, `D` | Move / Walk around |
| `Mouse` | Look around (captured automatically) |
| `Left Click` | Break targeted block |
| `Right Click` | Place selected block |
| `1` - `5` / `Scroll Wheel` | Select block material (Dirt, Stone, Wood, Sand, Leaves) |
| `Space` | Jump (Survival) / Fly Up (Creative) |
| `Left Shift` | Fly Down (Creative) |
| `Left Ctrl` | Sprint |
| `F` | Toggle between Survival (Walking) and Creative (Flight) |
| `Escape` | Toggle in-game Pause / Multiplayer Menu |
| `H` | Quick-Host Multiplayer Server (Port 25565) |
| `J` | Quick-Connect to Target Server IP |
| `F1` | Toggle Wireframe mode |
| `R` / `F5` | Hot-reload all GLSL shaders |
| `Q` | Quit game |

---

## Multiplayer Setup Guide (Tested & Verified)

We successfully connected across different PCs using **LogMeIn Hamachi** and **Windows Defender Firewall** configuration. Follow these exact steps to play together:

### 1. Configure Windows Firewall (Host PC)
Because home networks and Hamachi virtual adapters are often classified as **Public Networks**, Windows Firewall will block incoming connections by default unless allowed:
1. Open Windows Start menu, type **"Allow an app through Windows Firewall"**, and press Enter.
2. Click **Change settings** at the top right.
3. Locate **`VoxelEngine`** (or click *Allow another app...* and browse to `VoxelEngine.exe`).
4. Ensure **BOTH** the **Private** and **Public** checkboxes are **CHECKED**.
5. Click **OK**.

### 2. Connect via LogMeIn Hamachi
1. Both the Host and Client install and open **LogMeIn Hamachi**.
2. **Host**: Click *Network* -> *Create a new network* (enter a Network ID and Password).
3. **Friend (Client)**: Click *Network* -> *Join an existing network* (enter the Host's Network ID and Password).
4. Verify that you both see each other with a **solid green dot** in Hamachi.
5. In Hamachi, the host right-clicks their IPv4 address (starts with `25.x.x.x`) and clicks **"Copy IPv4 address"**, then shares it with the friend.

### 3. Configure and Connect in the Game
1. **Client**: Open the extracted game folder, open **`server.txt`**, paste the host's `25.x.x.x` Hamachi IPv4 address, and save.
2. **Host Starts First**:
   - Host launches `VoxelEngine.exe`.
   - Host presses `Escape` and clicks **`[ HOST SERVER ]`** (or presses hotkey **`H`**).
   - Verify the window title bar displays: **`[HOST: 1 Players]`**.
3. **Client Connects**:
   - Client launches `VoxelEngine.exe`.
   - Client presses `Escape` and clicks **`[ CONNECT: 25.x.x.x ]`** (or presses hotkey **`J`**).
   - Client's title bar switches to **`[CLIENT: Connected #2]`**, and the button turns green.
4. **Success!** Both players will see each other's 3D character avatars walking around, and any placed or broken blocks will synchronize instantly in real-time.

> **Diagnostic Logs**: If a connection fails, the in-game menu displays a red diagnostic banner, and full error details with recommended solutions are written to `network_log.txt`.

---

## Building from Source

### Prerequisites
- **CMake** 3.20+
- **C++20 Compiler**: Visual Studio 2022 (MSVC) with Windows 10/11 SDK, GCC 10+, or Clang 12+
- **OpenGL 4.3** capable GPU

### Quick Build & Package (Windows)
Run the automated build and distribution packaging script:
```cmd
package.bat
```
This automatically compiles an optimized Release build with statically linked C++ runtime (`/MT`) and packages everything into:
`VoxelEngine-Windows-x64.zip`

### Manual CMake Build
```bash
# Configure Release
cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release

# Build executable
cmake --build build_release --config Release

# Run
./build_release/VoxelEngine.exe
```

---

## Dependencies (Auto-fetched via CMake)

| Library | Version | Purpose |
|:---|:---|:---|
| [GLFW](https://github.com/glfw/glfw) | 3.4 | Window management, input handling & OpenGL context |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | Vector & matrix mathematics |
| [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) | Latest | SIMD-friendly procedural noise & cave generation |
| [stb](https://github.com/nothings/stb) | Latest | Image decoding & asset utilities |
| [Winsock2](https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-start-page-2) | System | Multi-threaded TCP client/server networking |

---

## License

MIT License. Free to use, modify, and distribute.
