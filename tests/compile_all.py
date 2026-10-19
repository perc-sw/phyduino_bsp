#!/usr/bin/env python3
"""Compile every test sketch and library example with arduino-cli.

Usage:
    python tests/compile_all.py [--fqbn FQBN] [--config-file arduino-cli.yaml]

Compiles tests/sketches/*/ and platform/libraries/*/examples/**/ for the given
board (default: phyduino:rp2:phyduino_pico) and prints a pass/fail summary.
A sketch that includes FreeRTOS.h is compiled with os=freertos added to the
board's options (Tools > Operating System > FreeRTOS), and one that includes
cmsis_os2.h with os=cmsis (FreeRTOS + CMSIS-RTOS2), since they cannot build
without them; a --fqbn that already has os=cmsis serves both. Exit status is non-zero if anything fails. Used locally and by CI.
"""
import argparse
import pathlib
import re
import subprocess
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent


def find_sketches():
    roots = [REPO / "tests" / "sketches", REPO / "platform" / "libraries"]
    sketches = set()
    for root in roots:
        for ino in root.rglob("*.ino"):
            if root.name == "libraries" and "examples" not in ino.parts:
                continue
            sketches.add(ino.parent)
    return sorted(sketches)


FREERTOS_INCLUDE = re.compile(r'#include\s*[<"]FreeRTOS\.h[>"]')
CMSIS_RTOS2_INCLUDE = re.compile(r'#include\s*[<"]cmsis_os2\.h[>"]')


def includes(sketch, pattern):
    return any(pattern.search(f.read_text(encoding="utf-8"))
               for f in sketch.glob("*.ino"))


def needed_os(sketch, fqbn):
    """The os= option the sketch needs that the FQBN lacks, or None."""
    if includes(sketch, CMSIS_RTOS2_INCLUDE):
        return None if "os=cmsis" in fqbn else "os=cmsis"
    if includes(sketch, FREERTOS_INCLUDE):
        return None if "os=freertos" in fqbn or "os=cmsis" in fqbn else "os=freertos"
    return None


def with_option(fqbn, option):
    """Adds menu option "key=value" to an FQBN, replacing that key if present."""
    parts = fqbn.split(":")
    key = option.split("=")[0]
    options = [o for o in parts[3].split(",") if o and not o.startswith(key + "=")] if len(parts) > 3 else []
    return ":".join(parts[:3] + [",".join(options + [option])])


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--fqbn", default="phyduino:rp2:phyduino_pico")
    parser.add_argument("--config-file", help="arduino-cli config (e.g. a scratch sketchbook)")
    args = parser.parse_args()

    base = ["arduino-cli"]
    if args.config_file:
        base += ["--config-file", args.config_file]

    failures = []
    for sketch in find_sketches():
        name = sketch.relative_to(REPO).as_posix()
        fqbn = args.fqbn
        option = needed_os(sketch, fqbn)
        if option:
            fqbn = with_option(fqbn, option)
            name += f" ({option})"
        result = subprocess.run(base + ["compile", "--fqbn", fqbn, str(sketch)],
                                capture_output=True, text=True)
        if result.returncode == 0:
            print(f"PASS  {name}")
        else:
            print(f"FAIL  {name}\n{result.stdout}{result.stderr}")
            failures.append(name)

    print(f"\n{len(failures)} failed" if failures else "\nAll sketches compiled.")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
