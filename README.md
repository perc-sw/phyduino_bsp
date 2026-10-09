# Phyduino Boards: Arduino board package

Arduino IDE support for the University of Toronto Department of Physics
student-workshop boards. It is a standalone Arduino core built directly on the
Raspberry Pi pico-sdk. Sketches can use the Arduino API and the pico-sdk
(`#include "hardware/pwm.h"`) side by side.

| Board | MCU | Status |
|---|---|---|
| [Phyduino Pico](docs/boards/phyduino_pico.html) | RP2040 | v0.1.0 (in progress) |
| Phyduino Pico2 | RP2350B | planned (v0.2.0) |

## Install (users)

1. Arduino IDE → **File → Preferences → Additional boards manager URLs**, add:
   `https://github.com/<owner>/<repo>/releases/latest/download/package_phyduino_index.json`
2. **Tools → Board → Boards Manager**, search "Phyduino", install.
3. **Tools → Board → Phyduino Boards → Phyduino Pico.**

Supported hosts: Windows x64, Linux x64/arm64, macOS Apple Silicon.

## Develop (maintainers)

```
git clone --recurse-submodules <repo-url>
# link platform/ into your sketchbook + set tool paths: docs/maintenance/dev-setup.html
python tests/compile_all.py                 # via arduino-cli
cmake --workflow --preset phyduino_pico     # via CMake, -Werror (docs/maintenance/cmake-verify.html)
cmake --workflow --preset host-tests        # PC-side unit tests (docs/maintenance/host-tests.html)
```

Start with **[docs/development/architecture.html](docs/development/architecture.html)**: how a sketch is
built and where everything lives. Recurring jobs have step-by-step recipes in
[docs/maintenance/](docs/maintenance/), and the reasons behind the design are
in [docs/decisions/](docs/decisions/).

```
platform/      what ships to users (the release zip is exactly this folder)
sdk-builder/   builds the pico-sdk into platform/system/<board>/ (committed)
CMakeLists.txt, cmake/   CMake verification build of platform/ (not shipped)
extern/        pinned upstream submodules: pico-sdk, ArduinoCore-API, doctest
package/       release script + tool download list
tests/         compile-test sketches, PC-side unit tests (tests/host), hardware checklist
docs/          architecture, board pages, maintenance recipes, decisions
```

## Roadmap

| Stage | Version | Scope |
|---|---|---|
| 1 | v0.1.0 | Phyduino Pico board package with integrated pico-sdk |
| 2 | v0.1.1 | FreeRTOS and multi-core |
| 3 | v0.2.0 | Phyduino Pico2 (RP2350B, Arm); libraries updated |
| 4 | v0.2.1 | RISC-V build option for Phyduino Pico2 |

Libraries and examples for the Analog Shield and Analog Shield2 live in their own
repository, not in this board package ([ADR 008](docs/decisions/008-shield-libraries-separate-repo.html)).

Changes per version: [CHANGELOG.md](CHANGELOG.md).

## Licence

GNU General Public License v3.0, see [LICENSE](LICENSE). Bundled upstream code keeps its own licence
(texts in [platform/licenses/](platform/licenses/)):
pico-sdk and TinyUSB (BSD-3-Clause / MIT), ArduinoCore-API (LGPL-2.1).
