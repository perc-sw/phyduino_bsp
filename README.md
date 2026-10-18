# Phyduino Boards: Arduino board package

[![CI](https://github.com/perc-sw/phyduino_bsp/actions/workflows/ci.yml/badge.svg)](https://github.com/perc-sw/phyduino_bsp/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/perc-sw/phyduino_bsp?include_prereleases&sort=semver)](https://github.com/perc-sw/phyduino_bsp/releases)
![Licence](https://img.shields.io/badge/licence-GPL--3.0-blue)
![Arduino IDE](https://img.shields.io/badge/Arduino_IDE-2.x-00979D?logo=arduino&logoColor=white)
![pico-sdk](https://img.shields.io/badge/pico--sdk-2.2.0-C51A4A?logo=raspberrypi&logoColor=white)
![MCU](https://img.shields.io/badge/MCU-RP2040-C51A4A)
![Hosts](https://img.shields.io/badge/hosts-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)

Arduino IDE support for the University of Toronto Department of Physics
student-workshop boards. It is a standalone Arduino core built directly on the
Raspberry Pi pico-sdk. Sketches can use the Arduino API and the pico-sdk
(`#include "hardware/pwm.h"`) side by side.

| Board | MCU | Status |
|---|---|---|
| [Phyduino Pico](docs/boards/phyduino_pico.html) | RP2040 | v0.1.0-rc.2 |
| Phyduino Pico2 | RP2350B | planned (v0.2.0) |

## Install

1. Arduino IDE → **File → Preferences → Additional boards manager URLs**, add:
   `https://github.com/perc-sw/phyduino_bsp/releases/latest/download/package_phyduino_index.json`
2. **Tools → Board → Boards Manager**, search "Phyduino", install.
3. **Tools → Board → Phyduino Boards → Phyduino Pico.**

Supported hosts: Windows x64, Linux x64/arm64, macOS Apple Silicon.

The URL works once v0.1.0 is published and the repository is public.
Pin map and board details: [docs/boards/phyduino_pico.html](docs/boards/phyduino_pico.html);
all documentation starts at [docs/index.html](docs/index.html).

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
