#!/usr/bin/env python3
"""Compile every test sketch and library example with arduino-cli.

Usage:
    python tests/compile_all.py [--fqbn FQBN] [--config-file arduino-cli.yaml]

Compiles tests/sketches/*/ and platform/libraries/*/examples/**/ for the given
board (default: phyduino:rp2:phyduino_pico) and prints a pass/fail summary.
Exit status is non-zero if anything fails. Used locally and by CI.
"""
import argparse
import pathlib
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
        result = subprocess.run(base + ["compile", "--fqbn", args.fqbn, str(sketch)],
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
