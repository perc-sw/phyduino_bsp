# Release a version

Users install from (always the newest release):
```
https://github.com/<owner>/<repo>/releases/latest/download/package_phyduino_index.json
```
(Arduino IDE → File → Preferences → Additional boards manager URLs.)

1. Make sure `main` is green in CI and `tests/hardware-checklist.md` passes on
   a real board.
2. Set the new version in **both** `VERSION` and `platform/platform.txt`
   (`version=`). `make_release.py` refuses to run if they differ.
3. Move the `CHANGELOG.md` "Unreleased" section under the new version heading.
4. Check `docs/architecture.md` still matches the layout (update it if files
   moved).
5. Commit, then tag and push:
   ```
   git tag v0.1.0
   git push origin v0.1.0
   ```
   `.github/workflows/release.yml` compiles everything, runs
   `package/make_release.py` and publishes the zip + index as release assets.
   Older versions stay in the index so users can roll back.
6. Check the release: in a clean Arduino IDE add the URL above, install
   "Phyduino Boards", compile and upload Blink.

## Dry run locally

```
python package/make_release.py --repo <owner>/<repo> --local
python -m http.server -d package/build 8000
```
Writes `package/build/` (git-ignored). Downloads the toolchains once to
compute checksums (cached in `package/build/cache/`). `--local` points the
index at `http://localhost:8000/`, so while the server runs you can test a real
install: add `http://localhost:8000/package_phyduino_index.json` to the IDE's
boards manager URLs. (arduino-cli cannot download archives from `file://` URLs.)

## Toolchain hosting

ARM's Windows toolchain zip has no single top-level folder, which the Board
Manager requires. `make_release.py` re-packs it (contents unchanged) and the
release workflow publishes it once on its own release, tagged with the
`host_tag` from `package/tools.json` (e.g. `tools-arm-none-eabi-gcc-15.2.rel1`).
Later releases download that hosted file to compute the checksum and never
re-pack it. When changing the toolchain version, change `host_tag` too.

Windows path length: GCC ignores include folders whose full path exceeds 260
characters. A normal install under `%LOCALAPPDATA%\Arduino15` is about 200
characters. Very long user names or a relocated Arduino data folder can break
`#include <cassert>`-style C++ headers; the fix is a shorter data folder.
