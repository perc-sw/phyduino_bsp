# Licences

The Phyduino Boards platform is licensed under the GNU General Public License
v3.0 (`LICENSE` at the root of the release zip).

It bundles third-party code under its own licence:

| Component | Where in the platform | Licence | File |
|---|---|---|---|
| Raspberry Pi pico-sdk | `system/*/` (headers, `libpico.a`, `boot2.o`, linker scripts) | BSD-3-Clause | `pico-sdk.txt` |
| TinyUSB (part of the pico-sdk) | `system/*/` | MIT | `tinyusb.txt` |
| ArduinoCore-API | `cores/phyduino/api/` | LGPL-2.1 | `ArduinoCore-API.txt` |
| FreeRTOS kernel | `system/*_freertos/` (headers, `libpico.a`) | MIT | `FreeRTOS-Kernel.txt` |
| FreeRTOS RP2040 port (Raspberry Pi) | `system/*_freertos/` | BSD-3-Clause | `FreeRTOS-RP2040-port.txt` |
| CMSIS-FreeRTOS (Arm): the CMSIS-RTOS2 wrapper | `system/*_freertos/lib/libcmsis_os2.a` | Apache-2.0 | `CMSIS-FreeRTOS.txt` |
| CMSIS_6 (Arm): the CMSIS-RTOS2 API header | `system/*_freertos/include/cmsis_rtos2/cmsis_os2.h` | Apache-2.0 | `CMSIS_6.txt` |

These files are copies from `extern/` (same versions as each bundle's `GENERATED.md` and
`cores/phyduino/api/UPSTREAM.md`), and the two CMSIS ones from `sdk-builder/cmsis-rtos2/`
(versions in its `README.md`). Refresh them when updating a submodule or those files.
