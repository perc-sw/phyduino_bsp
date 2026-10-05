# Update the pico-sdk (or regenerate the SDK bundle)

Do this to move to a new pico-sdk release, or after changing anything in
`sdk-builder/` (board header, `tusb_config.h`, `SDK_COMPONENTS`).

1. Move the submodule to the new tag (skip if only regenerating):
   ```
   git -C extern/pico-sdk fetch --tags
   git -C extern/pico-sdk checkout <tag>          # e.g. 2.3.0
   git -C extern/pico-sdk submodule update --init lib/tinyusb
   ```
2. Regenerate the bundle for every board (from `sdk-builder/`):
   ```
   cmake --workflow --preset phyduino_pico
   ```
   This rewrites `platform/system/phyduino_pico/` completely.
   If it stops with "Two SDK include dirs provide different files with the same
   name", the new SDK added a clashing header; the message names it.
3. Check what changed: `git status platform/system` and
   `git diff platform/system/*/compile_flags.txt platform/system/*/link_flags.txt`.
   New `--wrap` or define lines are normal after an SDK update.
4. `python tests/compile_all.py`. Fix any core code the new SDK broke.
5. Hardware-test (`tests/hardware-checklist.md`): at least Blink, Serial echo,
   upload via 1200-baud touch.
6. Refresh `platform/licenses/pico-sdk.txt` and `tinyusb.txt` from `extern/pico-sdk/`
   (`LICENSE.TXT`, `lib/tinyusb/LICENSE`) in case they changed.
7. Commit the submodule bump **and** `platform/system/` together. Note the SDK
   version in `CHANGELOG.md`.

If the Arm toolchain version changes too, update `package/tools.json` and the
`release:` in `.github/actions/setup-build/action.yml` to match.
