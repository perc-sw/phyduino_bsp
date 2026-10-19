# CMSIS-RTOS2 on FreeRTOS

Sources for `lib/libcmsis_os2.a` in the FreeRTOS SDK bundle
(`platform/system/<board>_freertos/`), linked when a sketch is built with
Tools > Operating System > FreeRTOS (SMP) + CMSIS-RTOS2. Design:
`docs/decisions/010-cmsis-rtos2.html`.

| Folder / file | From | Licence |
|---|---|---|
| `CMSIS-FreeRTOS/` (`cmsis_os2.c`, `freertos_os2.h`, `freertos_mpool.h`, `LICENSE`) | [ARM-software/CMSIS-FreeRTOS](https://github.com/ARM-software/CMSIS-FreeRTOS) `v11.3.0` (4dd0035148cdc106f892877c27cba77d1de717d2), `CMSIS/RTOS2/FreeRTOS/` | Apache-2.0 |
| `CMSIS_6/` (`cmsis_os2.h`, `os_tick.h`, `LICENSE`) | [ARM-software/CMSIS_6](https://github.com/ARM-software/CMSIS_6) `v6.4.0` (af1b110e46a1720349f6f125e386e126767699bd), `CMSIS/RTOS2/Include/` | Apache-2.0 |
| `os_tick_rp2040.c` | Phyduino: the OS Tick functions the wrapper uses, for the RP2040 port's SysTick | as the rest of the repository |

The ARM files are copied unchanged. They are copied rather than added as
submodules because the CMSIS-FreeRTOS repository also carries its own FreeRTOS
kernel, with paths too long for a default Git checkout on Windows.

To update: replace the files from the new tags, update the table, then
regenerate the FreeRTOS bundle (`docs/maintenance/update-pico-sdk.html`).
