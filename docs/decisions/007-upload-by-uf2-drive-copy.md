# 007: Upload by copying the UF2 to the BOOTSEL drive, picotool as fallback

**Decision.** After the 1200-baud touch reboots the board into BOOTSEL, the
upload tool (`platform/tools/uf2_upload.cmd` on Windows, `uf2_upload.sh` on
Linux/macOS) copies the sketch's `.uf2` to the BOOTSEL drive (`RPI-RP2`, or
`RP2350` later). If that drive is not mounted it tries `picotool load -x`, and
it retries both for 15 s.

**Why.** picotool needs a WinUSB driver for the RP2040's BOOTSEL interface on
Windows, which the RP2040 boot ROM does not get installed automatically; users
must run Zadig, which needs admin rights that students on lab PCs may not have.
Found on hardware: every IDE upload failed at the last step on a Windows PC
without Zadig. Copying to the drive works on any PC with no set-up. picotool
stays as the fallback for machines where the drive is not auto-mounted (e.g.
headless Linux with picotool's udev rules).

**Why scripts, not a program.** A batch file and a POSIX shell script need
nothing installed and are easy to read and change. Windows uses a batch file,
not a PowerShell script, because managed PCs may block PowerShell scripts by
execution policy; the batch file only runs a one-line PowerShell *command* to
look up the drive letter by volume label (which avoids "no disk" prompts from
empty card readers). `.gitattributes` keeps `.sh` files LF and `.cmd` files CRLF
whatever the machine's git settings.

**Cost.** Two small scripts to keep in step. A drive copy cannot verify the
written flash the way picotool can; the board reboots when the UF2 transfer
completes, and an incomplete file simply leaves it in BOOTSEL.
