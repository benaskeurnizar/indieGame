# bsp_renderer

A Quake-style BSP map renderer built from scratch in C/C++ — raw WinAPI window, OpenGL
rendering, and my own parser for the Quake 1 `.bsp` map format. No engine, no map SDK.

## What it does

It loads a compiled Quake `.bsp` file, walks its BSP tree, and renders the map with a
first-person camera you can walk around in — not just a viewer, there's basic player
movement and collision too.

- **BSP file parsing** (`src/load_bsp.cpp`) — reads the binary `.bsp` format directly: nodes,
  leaves, planes, faces, edges, textures (miptexes), the visibility list, the works. No
  existing Quake tooling or libraries involved, just the format spec and a hex editor.
- **BSP tree traversal** (`src/render_bsp.cpp`, `bsp_traversal`) — walks the tree each frame to
  figure out which leaf the camera is currently standing in, which is the basis for
  everything else (visibility, collision) the way the original Quake engine did it.
- **Face triangulation** (`src/triangulate.cpp`) — BSP faces are arbitrary convex polygons, so
  they get fan-triangulated before they can be handed to OpenGL as triangles.
- **Texturing** — face textures come straight from the `.bsp`'s embedded miptex data and the
  Quake palette (`data/palette.lmp`), with an optional override system
  (`materials/materials.txt`, see `materials/README.md`) to swap in real image files instead.
- **Lighting** (`src/lighting.cpp`) — builds a separate "tnode" tree from the BSP planes for
  fast line-of-sight tracing through the map, which is what the lighting pass uses to check
  whether a face is actually lit or in shadow. Same trick the original engine used for
  lighting and hit-tracing.
- **Player movement & collision** (`physics/p_move.cpp`) — walking, gravity, jumping, and
  collision against the BSP geometry, in the same spirit as Quake's `pmove.c`.
- **OpenGL pipeline** (`openGlPipeline/`, `renderer/`) — small, hand-written wrappers around
  shader compilation/linking and buffer setup, nothing pulled in from a rendering library.

## Controls

| Input | Action |
|---|---|
| `W` `A` `S` `D` | Move |
| Space | Jump |
| Hold left mouse button + drag | Look around |

## Project layout

```
bsp_renderer/
├── src/                   # entry point, game loop, BSP loading/rendering, lighting
│   ├── win32_main.cpp      # window creation, message loop
│   ├── game_code.cpp       # ties everything together, pulls in the rest via #include
│   ├── load_bsp.cpp        # .bsp file parsing
│   ├── render_bsp.cpp      # BSP tree traversal, triangulating + drawing faces, texturing
│   ├── triangulate.cpp     # fan triangulation for BSP faces
│   ├── lighting.cpp/.h     # tnode tree + line tracing for lighting
│   ├── Game.h               # World/Camera/Engine structs
│   ├── my_bsp.h             # .bsp format structs (nodes, leaves, faces, planes, ...)
│   ├── endian.h
│   ├── glad.c                # generated OpenGL loader
│   ├── temp.c                 # scratch/unused — see "Worth knowing" below
│   └── build.bat
├── physics/
│   ├── p_move.cpp           # player movement + collision against the BSP
│   └── move_def.h
├── openGlPipeline/          # shader compile/link, buffer setup
├── renderer/                 # small render-side helpers
├── shaders/
│   ├── vertex.txt
│   └── fragment.txt
├── maps/                     # sample .bsp maps to try it with
│   ├── map5.bsp
│   └── 1bsp7.bsp
├── data/
│   └── palette.lmp           # Quake palette, used to color textures without materials.txt
├── materials/                 # optional texture overrides — see materials/README.md
├── include/                   # third-party headers (not checked in — see include/README.md)
└── lib/                       # reserved for third-party libs (not checked in — see lib/README.md)
```

Same unity-build style as the rest of my projects: `win32_main.cpp` includes `game_code.cpp`,
which pulls in everything else — `load_bsp.cpp`, `render_bsp.cpp`, `lighting.cpp`,
`triangulate.cpp`, `physics/p_move.cpp`, `openGlPipeline/shaders.c`, `renderer/render.c` — via
`#include`. `build.bat` only ever compiles `win32_main.cpp` and `glad.c` directly.

## Building

Windows only, MSVC toolchain, run from a Developer Command Prompt for VS:

1. Set up `include/` (and optionally `lib/`) first — see `include/README.md` for what's
   needed (`glad`, `cglm`, `stb_image`).
2. From `src/`, run `build.bat`. Output goes to `src/builds/`.
3. Run the exe from inside `src/builds/` — the shader, map, and palette paths are all relative
   to that directory.

## Worth knowing

This is an older project I'm pushing mostly as-is, with a few fixes so it actually runs for
anyone who clones it rather than just on my machine:

- The palette and materials-list paths were hardcoded to my own drive (`E:\...`) — now
  relative, pointing at `data/palette.lmp` and `materials/materials.txt`.
- There was a debug feature in `load_bsp.cpp` that dumps decoded textures to `.ppm` files —
  it pointed at a folder that only existed on my machine, and worse, didn't check whether the
  file actually opened before writing to it, so it would've crashed on first run for anyone
  else. It's fixed to fail quietly now (skips the dump if the folder isn't there) — see the
  comment above the loop in `load_bsp.cpp` if you want to use it.
- `src/temp.c` is a leftover scratch file (a fragment of triangulation code I was messing
  with) — it's not part of the build, just kept around for reference.
- `materials/` ships empty. Without `materials.txt`, textures render straight from the
  `.bsp`'s built-in palette, which looks fine, just flatter than real textures would.

See `NextTODO.txt` for what I was planning to work on next — better lighting (shadows,
material reflection), non-face objects with their own physics, and possibly a Vulkan
rewrite of the renderer down the line.

## License

MIT — see [LICENSE](LICENSE).
