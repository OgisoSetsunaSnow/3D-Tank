# 3D Tank Render — Common Framework

A cross-platform 3D rendering and audio framework built as an internal training project. It renders a textured 3D tank model with a real-time FPS counter, instanced 2D/3D quad drawing, debug drawing utilities, and WAV audio playback.

Supported platforms:
- **macOS** — Metal renderer via CMake + Xcode
- **Windows PC** — DirectX 12 renderer via Visual Studio solution
- **PS4** — GNM/PSSL renderer via PS4 SDK (separate build)

---

## Table of Contents

1. [Hardware Requirements](#hardware-requirements)
2. [Software Requirements](#software-requirements)
3. [Project Structure](#project-structure)
4. [Building on macOS](#building-on-macos)
5. [Building on Windows](#building-on-windows)
6. [Running the Application](#running-the-application)
7. [Architecture Overview](#architecture-overview)

---

## Hardware Requirements

### macOS

The macOS build targets **Apple Silicon (arm64)** exclusively. Intel Macs are not supported.

**Minimum macOS version: 26.2 (Tahoe)**

| Machine | Minimum Supported Model |
|---|---|
| MacBook Air | M2 (2022) |
| MacBook Pro | M2 (2023) |
| Mac mini | M2 (2023) |
| iMac | M4 (2024) |
| Mac Studio | M2 Max / M2 Ultra (2023) |
| Mac Pro | M2 Ultra (2023) |

> M1 Macs are excluded because macOS 26 (Tahoe) does not run on them.

**GPU:** A Metal-capable GPU is required. All Apple M2 and later chips include an integrated Metal GPU. The renderer uses:
- Colour buffer: `MTLPixelFormatBGRA8Unorm_sRGB`
- Depth/stencil buffer: `MTLPixelFormatDepth32Float_Stencil8`

**Audio:** Standard audio output (built-in speakers or external). Uses CoreAudio `AudioQueue` for low-latency PCM WAV playback.

### Windows PC

- **GPU:** DirectX 12-capable GPU (NVIDIA GTX 900 series / AMD RX 400 series or later)
- **OS:** Windows 10 version 1903 or later (DirectX 12 required)
- **Audio:** DirectSound-compatible audio device

### PS4

- PlayStation 4 or PlayStation 4 Pro hardware
- PS4 SDK (licensed access required)

---

## Software Requirements

### macOS

| Tool | Minimum Version |
|---|---|
| macOS | 26.2 (Tahoe) |
| Xcode | 26.x |
| CMake | 3.21 |
| Apple Clang (via Xcode) | C++17 support required |

**System frameworks used** (provided by macOS/Xcode, no separate installation needed):

| Framework | Purpose |
|---|---|
| `Metal` | GPU rendering API |
| `MetalKit` | Metal view/texture helpers |
| `Cocoa` | Window and event management |
| `CoreAudio` / `AudioToolbox` | PCM audio playback via AudioQueue |
| `AVFoundation` | High-level audio/media |
| `Foundation` | Core Objective-C runtime |
| `QuartzCore` | `CAMetalLayer` for Metal presentation |

### Windows PC

| Tool | Minimum Version |
|---|---|
| Windows | 10 version 1903 |
| Visual Studio | 2019 or later (with C++ Desktop workload) |
| Windows SDK | 10.0.18362.0 or later |

---

## Project Structure

```
common_framework/
├── CMakeLists.txt                  # macOS/Xcode build (arm64 only)
├── InternalTraining_Framework.sln  # Windows/Visual Studio solution
├── framework/
│   ├── assets/
│   │   ├── font/       # BMFont descriptor + atlas
│   │   ├── mesh/       # OBJ mesh files
│   │   ├── pc_shader/  # HLSL shaders (DirectX 12)
│   │   ├── mac_shader/ # Metal shaders (MSL) — used by the macOS backend
│   │   ├── ps4_shader/ # PSSL shaders (PS4 GNM)
│   │   ├── sound/      # WAV audio files
│   │   └── tex/        # TGA texture files
│   ├── audio/
│   │   ├── AudioInterface.h        # Platform-agnostic audio API
│   │   ├── AudioResourceWave.cpp/h # WAV file loader
│   │   ├── Mac/AudioMac.mm         # CoreAudio/AudioQueue backend
│   │   ├── PC/AudioPC.cpp/h        # Windows audio backend
│   │   └── PS4/AudioPS4.cpp/h      # PS4 audio backend
│   ├── render/
│   │   ├── RenderInterface.h       # Platform-agnostic render API
│   │   ├── BMfont.cpp/h            # BMFont text rendering
│   │   ├── RenderMeshResource.cpp  # OBJ mesh loading
│   │   ├── RenderTextureResource.cpp # TGA texture loading
│   │   ├── tga.cpp/h               # TGA image parser
│   │   ├── tiny_obj_loader.cpp/h   # OBJ mesh parser
│   │   ├── Mac/                    # Metal backend
│   │   ├── PC/                     # DirectX 12 backend
│   │   └── PS4/                    # GNM backend
│   └── src/
│       ├── Mac/main_mac.cpp        # macOS entry point
│       ├── PC/main_PC.cpp          # Windows entry point
│       └── PS4/                    # PS4 entry point
```

---

## Building on macOS

**1. Generate the Xcode project:**

```bash
cmake -S . -B framework/build_xcode -G Xcode
```

**2. Open in Xcode:**

```bash
open framework/build_xcode/framework.xcodeproj
```

**3. Build and run** from Xcode using the `framework` scheme (Debug or Release), or build from the command line:

```bash
xcodebuild -project framework/build_xcode/framework.xcodeproj \
           -scheme framework \
           -configuration Debug \
           build
```

The working directory is set to `framework/` so all relative asset paths (e.g. `assets/mesh/plane.obj`) resolve correctly when launching from Xcode.

> To regenerate the Xcode project after changing `CMakeLists.txt`, re-run the `cmake` command above or use the `update_xcode.sh` script at the repo root.

---

## Building on Windows

Open `InternalTraining_Framework.sln` in Visual Studio 2019 or later and build the `framework` project in Debug or Release configuration.

The Windows backend uses DirectX 12 (HLSL shaders in `framework/assets/pc_shader/`).

---

## Running the Application

On launch the application:

1. Plays two WAV sound effects via CoreAudio (macOS) or DirectSound (Windows).
2. Opens a Cocoa/Win32 window and initialises the Metal/D3D12 renderer.
3. Loads `assets/mesh/plane.obj` and `assets/tex/TankTex.tga`.
4. Renders the textured mesh with a pulsing scale animation.
5. Displays a live FPS counter (60-frame rolling average) in the top-left corner.
6. Closes cleanly when the window is closed.

---

## Architecture Overview

The framework is structured around platform-agnostic interfaces with per-platform implementations selected at compile time:

| Interface | macOS impl | Windows impl | PS4 impl |
|---|---|---|---|
| `RenderInterface` | `RenderMac` (Metal) | `RenderPC` (D3D12) | `RenderPS4` (GNM) |
| `AudioInterface` | `AudioMac` (CoreAudio) | `AudioPC` | `AudioPS4` |

- **Render pipeline (macOS Metal):**
- Shaders are stored in an external Metal source file and compiled at runtime.
- The Metal source lives at `framework/assets/mac_shader/shaders.metal` and is loaded
- by the macOS renderer at startup. `CMakeLists.txt` defines a compile-time macro
- `SHADERS_PATH` that embeds the absolute path to this file into the binary so the
- app can reliably locate it when launched from Xcode or other environments.
- Ensure `framework/assets/mac_shader/shaders.metal` is present when running the
- built binary outside Xcode; regenerate the Xcode project with CMake after moving
- the repository or changing file layout so `SHADERS_PATH` remains valid.
- Pipeline State Objects (PSOs) are created on demand and cached by a key encoding blend mode, depth mode, and shader type.
- Triple-buffered rendering (`QUEUE_SLOT_COUNT = 3`) with a dispatch semaphore to throttle CPU ahead of GPU.
- Supported shader types: `OpaqueBasic` (non-instanced mesh), `Instance3D` (world-space quads), `Instance2D` (clip-space quads).

**Render pipeline (Windows D3D12):**
- Pre-compiled HLSL shaders in `framework/assets/pc_shader/`.
- Standard D3D12 resource/descriptor heap management.
