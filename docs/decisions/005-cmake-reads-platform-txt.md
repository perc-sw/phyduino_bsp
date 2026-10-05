# 005: The CMake verification build reads platform.txt / boards.txt

**Decision.** The root CMake project (`CMakeLists.txt`, `cmake/`) builds the
core, libraries and sketches outside the Arduino IDE. It gets its compiler and
link flags by reading `platform/platform.txt`, `boards.txt` and
`platform.local.txt` (`cmake/arduino_properties.cmake`), not from a copy.

**Why.** A second hand-written flag list would drift from the Arduino build,
and the CMake build would then "verify" something users never get. Reading
the same files keeps one source of truth; the identical ELF size for the
same sketch confirms it.

**Cost.** A small property expander in CMake. Recipes (`recipe.*.pattern`) are
not executed; the CMake link step mirrors `recipe.c.combine.pattern` by hand,
so a change to that recipe must be repeated in `cmake/phyduino.cmake`.
Nothing CMake-related goes into `platform/`, and `make_release.py` excludes
CMake files from the zip regardless.
