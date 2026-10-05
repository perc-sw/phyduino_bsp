# Add a board

Example: adding `phyduino_pico2`. Use `phyduino_pico` as the template at every step.

1. **SDK board header**: `sdk-builder/boards/<board>.h`. Copy
   `phyduino_pico.h`; set `pico_board_cmake_set(PICO_PLATFORM, ...)`, flash
   size, boot stage 2, crystal, SDK default UART/I2C/SPI/LED. SDK settings only:
   no Arduino pin map here.
2. **Preset**: add a configure/build/workflow preset named `<board>` to
   `sdk-builder/CMakePresets.json` (copy the `phyduino_pico` entries).
3. **Bundle**: from `sdk-builder/`, `cmake --workflow --preset <board>`
   → creates `platform/system/<board>/`.
4. **Variant**: `platform/variants/<board>/`
   - `pins_arduino.h`: the pin table comment, `PINS_COUNT`,
     `PHYDUINO_PIN_TO_GPIO`, named pins, Serial/SPI/Wire pins and instances,
     on-board peripheral pins.
   - `variant.cpp`: must define `initVariant()` and `variantAnalogRead()`
     (return 0 if the board has no external analog inputs).
5. **boards.txt**: copy the `phyduino_pico.*` block and rename the prefix.
   `build.variant` must equal the folder names from steps 3–4; set
   `build.chip` (`rp2040` / `rp2350`), `build.mcu`, `build.f_cpu`, sizes.
6. **Docs**: `docs/boards/<board>.md` (copy the Pico page's sections).
7. **Test**: `python tests/compile_all.py --fqbn phyduino:rp2:<board>`, then
   the hardware checklist on a real board.
8. Add a CI step compiling for the new FQBN (`.github/workflows/ci.yml`) and a
   `CHANGELOG.md` entry.
