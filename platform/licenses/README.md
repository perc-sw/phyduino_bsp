# Licences

The Phyduino Boards platform is licensed under the GNU General Public License
v3.0 (`LICENSE` at the root of the release zip).

It bundles third-party code under its own licence:

| Component | Where in the platform | Licence | File |
|---|---|---|---|
| Raspberry Pi pico-sdk | `system/*/` (headers, `libpico.a`, `boot2.o`, linker scripts) | BSD-3-Clause | `pico-sdk.txt` |
| TinyUSB (part of the pico-sdk) | `system/*/` | MIT | `tinyusb.txt` |
| ArduinoCore-API | `cores/phyduino/api/` | LGPL-2.1 | `ArduinoCore-API.txt` |

These files are copies from `extern/` (same versions as `GENERATED.md` and
`cores/phyduino/api/UPSTREAM.md`). Refresh them when updating a submodule.
