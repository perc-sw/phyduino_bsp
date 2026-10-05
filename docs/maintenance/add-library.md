# Add a library

Libraries in `platform/libraries/` ship with the platform and appear under
the board's examples in the IDE.

1. Create the standard Arduino layout:
   ```
   platform/libraries/<Name>/
     library.properties     copy from SPI/; set name, sentence, architectures=rp2
     src/<Name>.h           public API, with a header comment saying what it is for
     src/<Name>.cpp
     examples/<Example>/<Example>.ino
   ```
2. Take pins and bus instances from `pins_arduino.h` macros (never hard-code a
   GPIO), so the library works on every variant that defines them.
3. Use pico-sdk calls directly where that is simpler than going through the
   Arduino API; everything in the SDK bundle is available.
4. Every example must compile: `python tests/compile_all.py` picks up
   `examples/` automatically.
5. If the library needs an SDK component not yet in the bundle, add it to
   `SDK_COMPONENTS` in `sdk-builder/CMakeLists.txt` and follow
   `update-pico-sdk.md` from step 2.
6. Add hardware test steps to `tests/hardware-checklist.md` and a
   `CHANGELOG.md` entry.

Code that `analogRead()` or start-up depends on (on-board chips) belongs in
the variant, not a library: Arduino builds a library only when a sketch
includes it.
