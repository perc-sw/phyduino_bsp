@echo off
rem Uploads a UF2 file to a Phyduino board that is in BOOTSEL mode (Windows).
rem
rem Usage: uf2_upload.cmd <sketch.uf2> <picotool>
rem
rem Called by platform.txt after the IDE's 1200-baud touch has rebooted the
rem board into BOOTSEL. Retries for up to 15 s; on each try it
rem   1. copies the UF2 to the BOOTSEL drive (RPI-RP2 / RP2350) if it is
rem      mounted: needs no driver, works on any Windows PC, or
rem   2. runs "picotool load -x", which works where picotool's WinUSB driver
rem      is installed (Zadig).
rem The board reboots into the new sketch by itself when the copy completes.
setlocal EnableDelayedExpansion

set "UF2=%~1"
set "UF2=%UF2:/=\%"
set "PICOTOOL=%~2"
if not exist "%UF2%" (
    echo uf2_upload: file not found: %UF2%
    exit /b 2
)

for /L %%t in (1,1,15) do (
    rem Look for the BOOTSEL drive by its volume label (no disk prompts on empty drives).
    set "DRIVE="
    for /f "usebackq delims=" %%L in (`powershell -NoProfile -NonInteractive -Command "(Get-Volume -ErrorAction SilentlyContinue | Where-Object { $_.FileSystemLabel -eq 'RPI-RP2' -or $_.FileSystemLabel -eq 'RP2350' } | Select-Object -First 1).DriveLetter"`) do set "DRIVE=%%L"
    if defined DRIVE (
        echo Copying to BOOTSEL drive !DRIVE!: ...
        copy /y "%UF2%" "!DRIVE!:\" >nul
        if not errorlevel 1 (
            echo Upload complete; the board restarts into the new sketch.
            exit /b 0
        )
    )

    "%PICOTOOL%" load -x "%UF2%" >nul 2>&1
    if not errorlevel 1 (
        echo Upload complete via picotool.
        exit /b 0
    )

    rem Wait 1 s (ping works without a console, unlike "timeout").
    ping -n 2 127.0.0.1 >nul
)

echo uf2_upload: no board in BOOTSEL mode found after 15 s.
echo   - New board, or the sketch crashed: slide SW1 to GND, press reset, then upload again.
echo   - Or copy the .uf2 file to the RPI-RP2 drive by hand.
exit /b 1
