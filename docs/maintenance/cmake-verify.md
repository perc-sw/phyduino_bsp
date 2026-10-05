# CMake verification build

Builds the core, variant, libraries and every sketch outside the Arduino IDE.
Use it to check code with strict warnings, browse the code in an IDE (CLion,
VS Code + clangd), or get an ELF with a map file for debugging. It is **not**
shipped: the CMake files live outside `platform/`, and `make_release.py`
excludes CMake files from the zip in any case.

## Run it

From the repo root:
```
cmake --workflow --preset phyduino_pico
```
Output in `build/phyduino_pico/`: `<sketch>.elf`, `.map`, `.uf2` (if picotool
is found) and `compile_commands.json`. Build one sketch with
`cmake --build --preset phyduino_pico --target blink`.

Needs CMake ≥ 3.21, Ninja and the Arm toolchain. The compiler is found via
`-DPHYDUINO_TOOLCHAIN_PATH=...`, else `compiler.path` in
`platform/platform.local.txt` (the same file as the Arduino dev setup), else
`PATH`.

## What it checks

- The same sources as the Arduino build: `platform/cores/phyduino/**`,
  `platform/variants/<board>/`, `platform/libraries/*/src`.
- The same flags: read from `platform.txt` and `boards.txt` by
  `cmake/arduino_properties.cmake`, so there is no second copy to keep in step.
  (`-w` is dropped and `-Wall -Wextra -Werror` added for our code; vendored
  `cores/phyduino/api/` stays quiet.) Turn `-Werror` off with `-DPHYDUINO_WERROR=OFF`.
- The same link: SDK bundle `boot2.o`, `libpico.a` (whole-archive),
  `link_flags.txt`, `memmap.ld`.
- Every sketch in `tests/sketches/` and `platform/libraries/*/examples/`.
  The ELF matches the Arduino build's (e.g. same `arm-none-eabi-size` output).

## Differences from the Arduino builder

- No automatic function prototypes: in a sketch, define or declare functions
  before use. (Arduino accepts either order.)
- Libraries are linked when the sketch has `#include <Library.h>` for a
  library in `platform/libraries/`.
- Sketch folder names must be unique across tests and examples (they become
  target names).

## Files

| File | Role |
|---|---|
| `CMakeLists.txt`, `CMakePresets.json` | entry point, one preset per board |
| `cmake/arm-none-eabi.cmake` | toolchain file |
| `cmake/arduino_properties.cmake` | reads `platform.txt` / `boards.txt` / `platform.local.txt` |
| `cmake/phyduino.cmake` | `phyduino_setup()` (core + library targets), `phyduino_add_sketch()` |

Adding a board: add a preset with `PHYDUINO_BOARD` set to its id from
`boards.txt`. The separate `sdk-builder/` project (which builds the SDK bundle)
is unrelated.
