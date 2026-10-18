#!/usr/bin/env python3
"""Remind when enough changes have built up for a new pre-release tag.

Usage:
    python package/prerelease_reminder.py [--threshold N] [--always]

Counts the CHANGELOG.md bullets ("- ...") added since the newest v* tag. Every
feature or fix gets one bullet, so at N (default 10) or more it prints a
reminder with the bullets as a short description of the changes. Silent below
N unless --always. Run after each commit by .githooks/post-commit.
"""
import argparse
import pathlib
import re
import subprocess
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent


def git(*args):
    result = subprocess.run(["git", *args], cwd=REPO, capture_output=True,
                            text=True, encoding="utf-8")
    return result.stdout if result.returncode == 0 else None


def bullets(text):
    """Top-level "- " entries, with their indented continuation lines joined.
    Skips "### Open items" (status notes, not features or fixes)."""
    entries = []
    counted = True
    for line in text.splitlines():
        if line.startswith("#"):
            counted = not line.lower().startswith("### open")
        if not counted:
            continue
        if line.startswith("- "):
            entries.append(line[2:].strip())
        elif entries and entries[-1] and line.startswith("  ") and line.strip():
            entries[-1] += " " + line.strip()
        else:
            entries.append(None)  # blank line or heading ends the entry
    return [e for e in entries if e]


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--threshold", type=int, default=10)
    parser.add_argument("--always", action="store_true", help="print the count even below the threshold")
    args = parser.parse_args()
    sys.stdout.reconfigure(errors="replace")  # Windows consoles may not be UTF-8

    tag = (git("describe", "--tags", "--abbrev=0", "--match", "v*") or "").strip()
    if not tag:
        return 0
    old = set(bullets(git("show", f"{tag}:CHANGELOG.md") or ""))
    new = [b for b in bullets((REPO / "CHANGELOG.md").read_text(encoding="utf-8")) if b not in old]

    if len(new) < args.threshold:
        if args.always:
            print(f"{len(new)} CHANGELOG entries since {tag} (reminder at {args.threshold}).")
        return 0

    version = (REPO / "VERSION").read_text().strip()
    if f"v{version}" == tag:  # VERSION not bumped yet: suggest the next build number
        match = re.fullmatch(r"(.*-(?:alpha|rc))(?:\.(\d+))?", version)
        if match:
            version = f"{match.group(1)}.{int(match.group(2) or 0) + 1}"
    print(f"\n*** {len(new)} changes since {tag}: time for a pre-release tag (v{version}). ***")
    print("Changes:")
    for entry in new:
        print(f"  - {entry}")
    print("Recipe: docs/maintenance/release.html\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
