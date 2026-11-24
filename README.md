# Minecraft LittleTiles Reader

**English** | [简体中文](README.zh-CN.md)

Parse **Minecraft** save files (Anvil `.mca`), read the tile data of the
[Little Tiles](https://github.com/CreativeMD/LittleTiles) mod, and rebuild it as
a textured **OBJ** model (OBJ + MTL + PNG) or an editable **Blockbench Java
Block/Item project** (`.bbmodel`).

> **How development continues**  
> AI assistants are now capable enough that, given this project's existing
> architecture and engineering documentation, they can take over the remaining
> work. Most future code updates will therefore be made by me together with AI.

## Versions and Scope

This repository contains the **C++ core and CLI, 0.3.0-beta**. The companion
**desktop App 0.2.0** is a separate Windows x64 test build with a Flutter UI;
its version number and release cycle are independent of the core. macOS App
updates are deferred. Downloads: [LittleTiles Reader](https://www.inception.work/littletiles).

| Input | Current scope |
|---|---|
| Native Forge 1.12.2 saves | `DataVersion` 1343; legacy numeric IDs and metadata; the save's Forge registry can supply mod block IDs |
| Native Forge 1.20.1 saves | `DataVersion` 3465; complete namespaced block states and properties; matching v2 assets required for textured export |
| LittleTiles structure SNBT | Legacy `tiles` lists and modern `t` maps, including nested collections and per-level grids |

The current App workflow exposes **Overworld export only**. The core still has
other dimension parameters; their presence does not mean Nether or End export
has been validated in this release.

Only saves created and kept in the corresponding game/mod environment are
supported. **Upgraded, downgraded or migrated saves are unsupported.** The reader
does not upgrade or modify a save. Chunk metadata cannot prove creation history;
`source_history: "native"` is a caller's assertion, not a migration bypass.

The modern SNBT family also appears in 1.20/1.21 blueprints. Recognising that
family does not establish an exact Minecraft/LittleTiles version or add native
1.21 world support. Historical real-save checks and synthetic coverage are
recorded in [the 1.20.1 adaptation notes](docs/native-world-export-1.20.1-progress.md).

## Export Capabilities

- Region files (`.mca`) → chunk NBT → LittleTiles geometry, including tiles
  stored in child collections; raw `ChunkTileEntities::ReadChunkNbt` is also available.
- SNBT → OBJ with per-corner offsets where supported by the geometry builder;
  exported models are static, without door animations or interactions.
- SNBT → editable `java_block` `.bbmodel` projects, either whole or automatically
  split into single-block projects; see the restrictions below.
- Plain blocks use cube geometry. States listed in the prepared package's
  `cross_states.txt` use crossed planes for plants. This is **not** a general
  renderer for stairs, multipart models, custom loaders or block entities.
- UVs retain source texture phase across tile/cell boundaries and are independent
  of output centering/scaling. Tint and tile colours are baked into PNGs; animated
  textures use the first frame. Default tint colours are approximations, not
  per-biome sampling of the original world.
- OBJ writes MTL plus PNGs, with `map_d` for textures that contain transparency.
  Missing mappings are reported with face counts and, when present,
  `<name>_missing_materials.tsv`; affected faces use the white fallback. Missing
  Blockbench material mappings fail export instead of silently dropping geometry.
- The core consumes one prepared assets directory. The companion App handles
  importing and composing vanilla, mod and resource-pack sources per game version.
- CLI prompts are **English only**. The desktop App's seven UI languages are a
  separate feature.

## Blockbench Export

The CLI writes project format **5.0**, model format **`java_block`**. Use a
Blockbench version that can open that project format. It does not emit Java block
JSON directly; open the project in Blockbench to inspect/edit and export it.

- Only standard axis-aligned cuboids are accepted. Any corner-offset/slanted tile
  rejects the export; door animations, switches and interactions are not exported.
- **Both whole and split export require the entire input to occupy at most
  3×3×3 world-aligned block cells.** The bound counts occupied grid cells using
  source coordinates, not just geometric width. Splitting does not bypass it.
- `output.split: "none"` (default) exports one project. `"blocks"` exports
  `<name>_blocks/block_<x>_<y>_<z>.bbmodel` plus `blocks.bbmanifest.json`.
  Labels preserve game X/Y/Z axes; each local component is 0..2, with gaps retained.
- One game block is 16 model units; this is **not** a precision limit. Higher LT
  grid precision is preserved as fractional coordinates. `normalize_scale` must
  remain `false`. Whole-project centering translates the occupied cell grid to
  model centre `[8,8,8]`; split projects retain cell-local placement.
- The CLI writes tinted, first-frame PNGs beside each project in
  `<project>_textures/` and references them by relative paths. Keep the project and
  texture folder together when moving them. A cube crossing a texture cell may be
  split into several editable elements to retain texture repetition.
- These are static model resources, not automatic multiblock placement,
  collision or MCreator behaviour definitions.

The geometry preflight reports a JSON `valid` flag and `issues` before material
resolution. Check `valid`, not merely the process exit code; it does not validate
material availability.

```sh
./LittleTilesReader --version
./LittleTilesReader --capabilities
./LittleTilesReader --inspect-snbt House.txt --inspect-format bbmodel
./LittleTilesReader --job examples/bbmodel.json --progress json
```

Edit the paths in [examples/bbmodel.json](examples/bbmodel.json) before running it.
Add `"split": "blocks"` to its `output` object for the batch mode. The full job
contract is [docs/job.md](docs/job.md). [BB notes](docs/bbmodel-v1.md) and
[split notes](docs/bbmodel-blocks.md) retain historical implementation records;
the current CLI uses linked PNGs as described above.

## Test Data

The historical region archives used for verification **are not distributed with this library**
(all of them are Minecraft 1.12.2 + Little Tiles 1.5.66 saves); they ship with the
generator-side test-data package. The paths below are relative to that package's
`data/` folder — pass the absolute path when running.
Enter the folder, then a chunk coordinate and a scan radius:

| Folder | Chunk (x, z) | Recommended radius | Scan size | Baseline (Debug, plain blocks on, hidden faces culled) |
|---|---|---|---|---|
| `data/regions/test_region/` | **0, 0** | **1** | 3×3 chunks | 236 tiles → 4,881 faces → 0.3 MB OBJ, ~0.1 s |
| `data/regions/test_region_medim/` | **-136, 49** | **5** | 11×11 chunks | 55,561 tiles → 388,744 faces → 34 MB OBJ, ~9 s |
| `data/regions/test_region_large/` | **-7, -26** | **5** | 11×11 chunks | 324,427 tiles + 1,945,017 plain blocks → 2,036,139 faces → 178 MB OBJ, ~50 s |

These are historical core-library baselines, not performance results for the
Windows App 0.2.0. `tools/benchmark.py` on the generator/test-data side re-runs all three with these
parameters and records the result; past runs are kept in
[`docs/benchmark.md`](docs/benchmark.md) (the script is not part of this repo).

```sh
# region folder, chunk x, chunk z, radius, plain blocks, cull hidden faces,
# center the model, normalize scale, print progress and timing
# (for a structure file .txt/.struct the three coordinate/radius questions are skipped)
# <region root> = the data/ folder of the test-data package
printf "<region root>/regions/test_region_large\n-7\n-26\n5\ny\ny\ny\nn\ny\n" | ./LittleTilesReader
```

Converting a structure between the two generations (the result lands next to the
input unless `--out` says otherwise, and the job-file equivalent is
[`examples/snbt_convert.json`](examples/snbt_convert.json)):

```sh
# 1.12.2 structure -> 1.20
./LittleTilesReader --convert 1.20 --input House.txt

# 1.20 blueprint -> 1.12.2
./LittleTilesReader --convert 1.12.2 --input House.struct --out House_112.txt
```

All prompts, progress and result lines are **English only** (to avoid mojibake from
terminal encodings). Every user-visible string goes through `galib::Tr()`, which is
now a single-argument pass-through — restoring another language means changing that
one function. The last question controls both the progress output
(`[progress] chunk (x, z) - i/n`, plus the per-chunk/per-block detail from the
library) and the final total-time line. Answer `n` for clean, script-friendly output.

The result is written to `outputs/` relative to the current working
directory (OBJ + MTL + a `<obj name>_textures/` folder).

## Assets and Resource Packs

The core reads a prepared **assets package directory** through `assets_root` / the
job's `assets.package`; it does not import Minecraft client JARs, mods or resource
packs directly. The companion App/generator resolves and composes those sources.
The App requires the matching vanilla client JAR before adding optional mods and
packs, and keeps the two game versions' assets separate.

A package contains `block_textures.tsv`, read-only `textures/` and, for modern
assets, versioned `manifest.json`. Textured 1.20.1 world export requires a matching
1.20.1 **format v2** package with full block-state and face-UV mappings. Legacy
packages remain available for the 1.12.2 path. Mod textures require a usable mapping;
importing a mod does not automatically implement its custom model renderer.

Choose a package in a job, through `LITTLETILES_ASSETS=<assets directory>`, or at the
interactive `assets root` prompt. Relative paths are resolved against the current
working directory, then the executable's directory and its parent.
See [the assets contract](docs/assets-package.md),
[texture mapping](docs/texture-mapping.md) and
[static animation-frame handling](docs/texture-animation-static-frame.md).

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
ctest --test-dir cmake-build-debug --output-on-failure
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

- Save profiles are restricted to **1.12.2 / 1.20.1**. Unsupported `DataVersion`
  values, selected-version conflicts and mixed-version chunks are rejected.
  Missing `DataVersion` permits a warned layout inference only; it cannot verify
  the precise version or native history. Modern height follows stored sections.
  MCA chunk compression is currently limited to zlib (type 2).
- Windows geometry uses `float`, while macOS/Linux use `double`
  (`CgalTypeDef.h`); cross-platform results may differ slightly.
- Plain blocks outside the supported cube/cross-plane paths are approximations.
  Arbitrary mod models, CTM/connected textures, dynamic rendering, shaders and
  exact in-game lighting are not reproduced.
- Texture alpha is exported, but how transparency appears depends on the target
  application's material/import settings. Default vegetation/water tint is not
  a reconstruction of world-specific biome colours.
- Structure SNBT conversion is distinct from save migration. During SNBT
  conversion, door animation/switch payloads may be passed through, but model
  export does not recreate those behaviours; see [SNBT notes](docs/snbt-format.md).
- Blockbench retains the cuboid, 3³-cell and no-normalisation restrictions above.
  Geometry inspection, project generation and actual in-game acceptance are
  separate checks; automated fixtures do not validate every mod combination.

## Matlab Support

The geometric structure data exported by the LittleTile mod can be converted with
`python/IntArrayInterpreter.py` from the generator-side test-data package to
display it in MatLab.

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

Engineering notes live in [`docs/`](docs/). Some documents retain dated plans or
validation records; use the current code and this README to distinguish them
from implemented capabilities. Useful entry points:

- [`docs/architecture.md`](docs/architecture.md) — module layout, the real call chain, ownership/thread-safety status, build
- [`docs/nbt-format.md`](docs/nbt-format.md) — `.mca` / chunk NBT / LittleTiles tile format, offset bit layout, coordinate system
- [`docs/snbt-format.md`](docs/snbt-format.md) — LittleTiles structure SNBT (both dialects), box encoding, children, mod textures
- [`docs/known-issues.md`](docs/known-issues.md) — verified defects, output determinism, test-data baselines, README/code mismatches
- [`docs/texture-mapping.md`](docs/texture-mapping.md) — UV conventions, tint baking, resource pack overlay
- [`docs/benchmark.md`](docs/benchmark.md) — recorded export baselines (generated by the test-data side's `tools/benchmark.py`)
- [`docs/reference-lt3d-importer.md`](docs/reference-lt3d-importer.md) — study of the third-party LT 3D Importer & Exporter mod (UV/texture strategies)

- [`docs/job.md`](docs/job.md) — non-interactive jobs and JSON progress events
- [`docs/build-guide.md`](docs/build-guide.md) — platform build instructions
- [`docs/release-and-licensing.md`](docs/release-and-licensing.md) — release and third-party licence requirements
