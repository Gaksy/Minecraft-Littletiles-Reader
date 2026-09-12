# Minecraft LittleTiles Reader

**English** | [简体中文](README.zh-CN.md)

Parse **Minecraft** save files (Anvil `.mca`), read the tile data of the
[Little Tiles](https://github.com/CreativeMD/LittleTiles) mod, and rebuild it as
a standard **OBJ** model (OBJ + MTL + textures) that opens directly in Blender
and other modelling tools.

> **How development continues**  
> AI assistants are now capable enough that, given this project's existing
> architecture and engineering documentation, they can take over the remaining
> work. Most future code updates will therefore be made by me together with AI.

## Current Status

- Reads region archives (`.mca`) → chunk NBT → LittleTiles tile entities
- Raw NBT entry point (`ChunkTileEntities::ReadChunkNbt`), so data that never
  went through an mca file can be parsed as well
- Rebuilds tile geometry, including LittleTiles' per-corner angle offsets and
  out-of-block tiles (clipped to the block box)
- Exports plain (non-LittleTiles) blocks as full cubes, with optional culling of
  faces hidden by neighbouring blocks
- Six-face UVs; biome tint and per-tile colours are baked into the exported PNGs
- Optional custom resource pack (folder / zip / rar) layered over the vanilla assets
- Chinese and English interface

## Tested Environment

- **Minecraft 1.12.2**
- **Little Tiles 1.5.66**

## Test Data

The region archives used for verification are kept in the repository
(all of them are Minecraft 1.12.2 + Little Tiles 1.5.66 saves).
Enter the folder, then a chunk coordinate and a scan radius:

| Folder | Chunk (x, z) | Recommended radius | Scan size | Baseline (Debug, plain blocks on, hidden faces culled) |
|---|---|---|---|---|
| `test_region/` | **0, 0** | **1** | 3×3 chunks | 236 tiles → 4,881 faces → 0.3 MB OBJ, ~0.1 s |
| `test_region_medim/` | **-136, 49** | **5** | 11×11 chunks | 55,561 tiles → 388,744 faces → 34 MB OBJ, ~9 s |
| `test_region_large/` | **-7, -26** | **5** | 11×11 chunks | 324,427 tiles + 1,945,017 plain blocks → 2,036,139 faces → 178 MB OBJ, ~50 s |

`python3 tools/benchmark.py` re-runs all three with these parameters and records
the result; past runs are kept in [`docs/benchmark.md`](docs/benchmark.md).

```sh
# language (1 = zh-CN, 2 = en-US), region folder, chunk x, chunk z, radius,
# plain blocks, cull hidden faces, center the model, normalize scale,
# print progress and timing
printf "2\ntest_region_large\n-7\n-26\n5\ny\ny\ny\nn\ny\n" | ./LittleTilesReader
```

The first question picks the interface language (blank = zh-CN); every prompt and
message is bilingual. The last question controls both the progress output
(`[progress] chunk (x, z) - i/n`, plus the per-chunk/per-block detail from the
library) and the final total-time line. Answer `n` for clean, script-friendly
output.

The result is written to `out_file/` relative to the current working
directory (OBJ + MTL + a `<obj name>_textures/` folder).

## Using a Resource Pack

Textures come from an assets root (`assets/1.12.2` by default). To export with a
resource pack, merge it onto the vanilla assets first:

```sh
python3 tools/build_assets_from_pack.py --pack "texture/MyPack.zip" --out assets/pack
```

Then point the reader at the result, either via `LITTLETILES_ASSETS=assets/pack`
or by typing the path at the `assets root (blank = auto-detect):` prompt.
The pack overrides only the textures it ships; the rest falls back to vanilla.
Relative paths are resolved against the current directory, then the executable's
directory and its parent (CLion runs with the build directory as its working
directory), and the chosen root is printed as an absolute path.
Details and limits: [`docs/texture-mapping.md`](docs/texture-mapping.md).

## Processing Workflow

Once the target chunk coordinates are provided, the approximate processing steps
are:

1. Compute the corresponding region coordinates
2. Read the `mca` binary file to obtain the 8KiB region index
3. Calculate the corresponding chunk index and read its compressed binary data
4. Decompress using [boost](https://archives.boost.io) and [zlib](https://zlib.net/)
5. Parse the data with [libnbt++](https://github.com/ljfa-ag/libnbtplusplus)
6. Pass the chunk's root NBT data to the **Little Tiles parser** for interpretation
7. Use [CGAL](https://www.cgal.org/) to construct the geometry of the Little Tiles data
8. Finally, use the **OBJ file builder** to export OBJ (with MTL and textures)

## Usage

Dependencies are managed by **vcpkg** (Homebrew is used as a fallback on macOS
when vcpkg is not available).

```sh
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug
```

### Windows

1. Use vcpkg to manage the zlib, boost, cgal and libnbt++ dependencies.
2. Configure the `VCPKG_ROOT` system environment variable.
3. Since `VCPKG_ROOT` uses MSVC for compilation, the project should also be
   compiled with MSVC.
4. Clone the project and configure the toolchain and CMake.
5. Execute `CMakeLists.txt`, then build `libnbt++` to fulfil the dependency
   (this project relies on that library for NBT parsing).
6. Finally, compile and run LittleTilesReader.

### macOS

1. Use Homebrew to install the dependencies:
    ```
    brew install boost
    brew install zlib
    brew install cgal
    ```
2. Clone the project.
3. Run CMake; it will automatically check for required dependencies.
4. Compile and run LittleTilesReader.

## Development Plan

- [x] Use CMake for project management
- [x] Refactoring
- [x] UV, materials and texture baking
- [ ] Normal export
- [ ] glTF / GLB export
- [ ] Library API for server-side use

## Language and Toolchain

- C++ Standard: **ISO C++17** (the code uses nested namespace definitions,
  if-init statements and `std::filesystem`, and CGAL 6.x requires C++17; the
  C++14 stated in earlier documentation did not match the code)
- Platform Toolset: Visual Studio 2022 (v143) / MSVC; Apple Clang on macOS
- Windows SDK Version: 10.0
- ⚠️ Do not use the MinGW toolchain

## Library Dependencies

Managed by vcpkg (Homebrew fallback on macOS); libnbt++ is pinned to a fixed
commit so builds stay reproducible.

| Library | Version | Used for |
|---|---|---|
| boost | 1.92.0 (CMake requires ≥ 1.74) | zlib filter of iostreams |
| zlib | 1.3.2 | chunk decompression |
| libnbt++ | commit `687e4303` | NBT parsing |
| CGAL | 6.2.1 | meshes and geometric operations |

## Known Limitations

- **Cross-platform floating point**: `FloatType` is currently `float` on Windows
  and `double` on macOS (`CgalTypeDef.h`), so geometry may differ slightly
  between the two platforms. It should be unified to `double` before
  server-side use, so that the same input always yields the same output.
- Only the 1.12 save layout is supported (`Sections` under `Level`, zlib
  compression type).
- MCPatcher/CTM connected textures inside a resource pack are not supported yet;
  the base texture of the connected block is used.

## Matlab Support

The geometric structure data exported by the LittleTile mod can be converted
with `IntArrayInterpreter.py` in the `python/` directory to display it in MatLab.

For example, the SNBT
`[I;0,0,0,2,1,2,-2135499923,-65537,65537,-65537,65537,-1,131073,-1,131073]` from:
```
{tiles:[{bBox:[I;0,0,0,2,1,2,-2135499923,-65537,65537,-65537,65537,-1,131073,-1,131073],tile:{block:"minecraft:stone"}}],min:[I;0,0,0],size:[I;2,1,2],grid:2,count:1}
```
When provided to this script, it outputs the corresponding MatLab code:
```
% Define grid size
grid_type = 2;

% Define the eight vertices of the cube
block_aabb = LtBlock(0,0,0,2,2,2);
block = LtBlock(0,0,0,2,1,2);
block.applyOffset(AngleID.WDS, LtPoint(2, 0, 1));
block.applyOffset(AngleID.WDN, LtPoint(-1, 0, -1));
block.applyOffset(AngleID.WUS, LtPoint(2, 0, 1));
block.applyOffset(AngleID.WUN, LtPoint(-1, 0, -1));
block.applyOffset(AngleID.EDS, LtPoint(1, 0, 1));
block.applyOffset(AngleID.EDN, LtPoint(-2, 0, -1));
block.applyOffset(AngleID.EUS, LtPoint(1, 0, 1));
block.applyOffset(AngleID.EUN, LtPoint(-2, 0, -1));
% Plot
showGrid(grid_type, -2, 2);
patchLtBlock(block, 'green', 0.7);
patchLtBlock(block_aabb, 'blue', 0.1);
```
<img width="628" height="567" alt="Screenshot 2025-09-01 at 13 40 34" src="https://github.com/user-attachments/assets/89748dc6-6f5f-43a5-949d-14306b0356d5" />

## Export Example

![image](https://github.com/user-attachments/assets/034008b4-f20e-424f-8a9d-377b32a4b70a)
![image](https://github.com/user-attachments/assets/23f98b62-a88a-4360-82e0-9f0e506f7876)

## Engineering Documentation

Detailed engineering notes live in [`docs/`](docs/) (verified against the code,
referencing symbols rather than line numbers):

- [`docs/architecture.md`](docs/architecture.md) — module layout, the real call chain, ownership/thread-safety status, build
- [`docs/nbt-format.md`](docs/nbt-format.md) — `.mca` / chunk NBT / LittleTiles tile format, offset bit layout, coordinate system
- [`docs/known-issues.md`](docs/known-issues.md) — verified defects, output determinism, test-data baselines, README/code mismatches
- [`docs/texture-mapping.md`](docs/texture-mapping.md) — UV conventions, tint baking, resource pack overlay
- [`docs/benchmark.md`](docs/benchmark.md) — recorded export baselines (`python3 tools/benchmark.py --write docs/benchmark.md`)
- [`docs/reference-lt3d-importer.md`](docs/reference-lt3d-importer.md) — study of the third-party LT 3D Importer & Exporter mod (UV/texture strategies)
