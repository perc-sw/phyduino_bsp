# Dev setup: use this checkout in the Arduino IDE

The SDK bundle is committed, so no build step is needed to use the core.
Only `sdk-builder/` needs the submodules.

## Once per machine

1. Install the [Arm GNU toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
   (`arm-none-eabi`), [picotool](https://github.com/raspberrypi/pico-sdk-tools/releases),
   Arduino IDE 2.x and/or `arduino-cli`.
   For `sdk-builder/` also: CMake ≥ 3.20, Ninja, Python 3.
2. Clone with submodules:
   ```
   git clone --recurse-submodules <repo-url>
   ```
   (Already cloned? `git submodule update --init --recursive`.)
3. Link `platform/` into your sketchbook as `hardware/phyduino/rp2`
   (Windows, in `cmd`):
   ```
   mklink /J "%USERPROFILE%\Documents\Arduino\hardware\phyduino\rp2" "C:\repos\student_workshop\phyduino_bsp\platform"
   ```
   Linux/macOS: `ln -s "$PWD/platform" ~/Arduino/hardware/phyduino/rp2`
4. Create `platform/platform.local.txt` (git-ignored) with your tool paths:
   ```
   compiler.path=C:/Program Files (x86)/Arm/GNU Toolchain mingw-w64-i686-arm-none-eabi/bin/
   tools.picotool.path=C:/Program Files (x86)/picotool
   ```
   `compiler.path` must end with `/`.
5. Restart the IDE. **Tools → Board → Phyduino Boards (in sketchbook) → Phyduino Pico.**
   If the Board Manager version is also installed, the IDE lists both; pick
   the "(in sketchbook)" one.

## Every change

```
python tests/compile_all.py
```
compiles every test sketch and library example through arduino-cli, and
```
cmake --workflow --preset phyduino_pico
```
builds the same code through CMake with `-Werror` (`cmake-verify.md`), and
```
cmake --workflow --preset host-tests
```
runs the PC-side unit tests (`host-tests.md`). Then upload the affected
sketches to a board and run the relevant items in `tests/hardware-checklist.md`.

## First upload to a new board

A board that has never run this core does not answer the 1200-baud touch:
slide SW1 to GND (BOOTSEL), press reset, upload, slide SW1 back to 3.3 V.
After that, uploads work from the IDE with no buttons.

Uploads need no USB driver: the upload script copies the sketch to the
RPI-RP2 drive (ADR 007). On Windows, picotool's own upload path additionally
needs a WinUSB driver (Zadig); it is only a fallback and is not required.
