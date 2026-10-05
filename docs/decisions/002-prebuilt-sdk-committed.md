# 002: Prebuilt SDK bundle, committed to git

**Decision.** `sdk-builder/` compiles the pico-sdk once per board into
`platform/system/<board>/` (libpico.a, merged headers, flag files), and that
output is committed.

**Why.**
- Arduino builds stay fast and need only the ARM compiler; no CMake for users.
- Clone and go: the IDE works from a fresh checkout, even years later, without
  rebuilding the SDK on a matching toolchain.
- Flag lists are generated from the SDK's own CMake (via a "probe" target)
  instead of hand-copied into `platform.txt`, which was the main maintenance
  pain in the first attempt (`C:\repos\arduino-phyduino`).
- An SDK update shows up as a reviewable diff of `compile_flags.txt` /
  `link_flags.txt`.

**Cost.** About 9 MB per board in git, regenerated on SDK updates. Generated
files must never be hand-edited (`GENERATED.md` in each bundle says so).
