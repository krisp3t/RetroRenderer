C++ real-time software rasterizer. The primary goal is to deepen understanding of 3D graphics fundamentals by implementing the rendering pipeline directly in software, with an OpenGL path available as a hardware-rendering comparison.

The project is a work in progress and is actively being developed.

## Screenshot

![Screenshot 2024-12-08 230126](https://github.com/user-attachments/assets/0177bdb7-dbbd-4d12-adbf-01671a34f70f)

## Current Status

The current baseline is a desktop-first renderer with:

- Software and OpenGL rendering backends.
- Runtime renderer configuration through Dear ImGui.
- Lightweight OBJ scene loading.
- Scene graph inspection and transform editing.
- Free-moving and orbit-style camera controls.
- Retro rendering presets for default, PICO-8, picoCAD, and PS1-style output.
- Golden hash, integration, rasterizer, importer, concurrency, and sanitizer smoke tests.
- CMake presets for Windows, Linux, macOS, Android, and Web/Emscripten targets.

## Features

- Frustum culling, backface culling, depth testing, raster clipping, and geometric clipping.
- Triangle, line, and point rasterization modes.
- Scanline, barycentric, and Pineda-style software fill paths.
- Perspective-correct and affine texture mapping.
- Flat face lighting, Gouraud-style shading, Phong/Blinn-Phong material controls, and point lights.
- Palette reduction, ordered dithering, color ramps, RGB555 quantization, fog, texture CLUT-style reduction, and PS1-style semitransparency controls.
- Skybox, grid gizmo, light gizmos, metrics overlay, and software worker stats.

## Supported Platforms

Primary development targets:

- Windows x64 with Clang, Ninja, CMake, and vcpkg.
- Linux x64 with Clang, Ninja, CMake, and vcpkg.

Additional targets:

- macOS presets exist but should be treated as less frequently exercised.
- Web/Emscripten builds are supported as a demo target.
- Android build glue exists and should be treated as experimental until tested regularly.

## Importer Baseline

The supported runtime scene format is currently OBJ through `LightweightObjSceneImporter`.

Assimp is not part of the active runtime importer path in the current source tree. If broader model format support is added later, it should be reintroduced behind an explicit CMake option and covered by tests.

## Build

This project uses CMake and vendored vcpkg. Presets are defined in `CMakePresets.json`.

Windows:

```bash
cmake --preset debug-x64-windows
cmake --build out/build/debug-x64-windows
```

Linux:

```bash
cmake --preset debug-x64-linux
cmake --build out/build/debug-x64-linux
```

Web/Emscripten:

```bash
cmake --preset wasm32-emscripten
cmake --build out/build/wasm32-emscripten
```

Run the executable from the repository root so `assets/` resolves correctly.

## Tests

Automated tests are built when `RETRO_BUILD_TESTS=ON`. The current test suite covers the software rasterizer, OBJ importer, importer-to-rasterizer integration, golden framebuffer hashes, concurrency behavior, and sanitizer instrumentation.

Windows tests:

```bash
cmake --preset debug-x64-windows
cmake --build --preset build-debug-x64-windows-tests
ctest --preset test-debug-x64-windows-tests
```

Linux sanitizer test lanes:

```bash
cmake --preset debug-x64-linux-asan
cmake --build --preset build-debug-x64-linux-asan-tests
ctest --preset test-debug-x64-linux-asan
```

```bash
cmake --preset debug-x64-linux-ubsan
cmake --build --preset build-debug-x64-linux-ubsan-tests
ctest --preset test-debug-x64-linux-ubsan
```

```bash
cmake --preset debug-x64-linux-tsan
cmake --build --preset build-debug-x64-linux-tsan-tests
ctest --preset test-debug-x64-linux-tsan
```

Golden hash baselines live in `tests/golden/`. On non-Windows platforms they run by default. On Windows, the test presets set `RETRO_RUN_GOLDEN_WINDOWS=1` for the regular test lane; ASan skips golden enforcement so that lane focuses on memory correctness.

Sanitizer details are documented in `docs/testing-sanitizers.md`.

## Visual Checks

Manual visual checks live under `assets/tests-visual/`. Add a README next to each new scene describing setup steps, target preset, and expected artifacts.

When rendering output changes intentionally, include before/after screenshots or GIFs in the PR and update golden hashes only after reviewing the change.

## CI

The GitHub Actions workflow in `.github/workflows/cmake-tests.yml` is intended to cover:

- Linux desktop tests.
- Linux ASan, UBSan, and TSan lanes.
- Windows desktop tests.

Keep the local CMake presets and CI commands aligned when changing build options.

## Known Limitations

- Scene loading is OBJ-only in the active runtime importer path.
- Mesh and texture scene data are CPU-side; GL/GLES mesh, texture, and frame-output resources are owned by renderer/presenter code.
- `RenderSystem` still coordinates backend selection, software worker scheduling, and presentation, so it is not fully backend-neutral yet.
- `Engine::Get()` is used widely across scene, renderer, material, and UI code.
- Web and Android paths are useful but not as heavily validated as desktop.
- There is no configured formatter; follow the existing C++ style.

## References

- [Joey de Vries: LearnOpenGL](https://learnopengl.com/)
- [ChiliTomatoNoodle: 3D Programming Fundamentals](https://www.youtube.com/watch?v=uehGqieEbus&list=PLqCJpWy5Fohe8ucwhksiv9hTF5sfid8lA)
- [Dmitry V. Sokolov: TinyRenderer](https://github.com/ssloy/tinyrenderer)
- [Jean-Colas Prunier: Scratchapixel](https://www.scratchapixel.com/)

## Assets

- [Seamless Sky Backgrounds](https://opengameart.org/content/seamless-sky-backgrounds)
