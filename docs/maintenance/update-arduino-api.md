# Update ArduinoCore-API

`platform/cores/phyduino/api/` is an unmodified copy of `api/` from
ArduinoCore-API (Arduino only compiles files inside the core folder, so a
submodule elsewhere would not be built).

1. Move the submodule to the new tag:
   ```
   git -C extern/ArduinoCore-API fetch --tags
   git -C extern/ArduinoCore-API checkout <tag>
   ```
2. Replace the copy (from the repo root):
   ```
   rm -rf platform/cores/phyduino/api
   cp -r extern/ArduinoCore-API/api platform/cores/phyduino/api
   ```
3. Re-create `platform/cores/phyduino/api/UPSTREAM.md` with the new tag and
   commit (`git -C extern/ArduinoCore-API rev-parse HEAD`). Copy the previous
   file's wording.
4. `python tests/compile_all.py`. If the API added new pure-virtual methods or
   functions the core must provide, implement them in the matching core file.
5. Refresh `platform/licenses/ArduinoCore-API.txt` from `extern/ArduinoCore-API/LICENSE`.
6. Commit the submodule bump and the new copy together; note it in `CHANGELOG.md`.
