# ESP32 Firmware Setup (Team)

This project supports two workflows:

1. Local ESP-IDF on host machine (recommended for flashing)
2. VS Code Dev Container (recommended for consistent build environment)

## Quick Rule

- Build in container if you want consistency.
- Flash from host if you want easy `COM3` access on Windows.

## 1) Host Setup (Windows)

1. Install ESP-IDF extension in VS Code.
2. Open `ESP32-firmware` folder in VS Code (not in container).
3. Run `ESP-IDF: Configure ESP-IDF extension`.
4. Select your ESP-IDF path.
5. Set serial port to `COM3` (or your board port).
6. Run in terminal:

```bash
idf.py reconfigure
idf.py build
idf.py -p COM3 flash monitor
```

## 2) Dev Container Setup

1. Install Docker Desktop.
2. Install VS Code extension: `Dev Containers`.
3. Open repo in VS Code.
4. Run `Dev Containers: Rebuild and Reopen in Container`.
5. In container terminal:

```bash
idf.py --version
idf.py reconfigure
idf.py build
```

Expected IDF version: `v5.5.3`.

## Why COM Port May Be Missing in Container

- `COM3` is a Windows host port.
- Containers use Linux device names (`/dev/ttyUSB0`, `/dev/ttyACM0`).
- If USB device is not passed to container, no serial port appears.

For beginners, use container for build and host for flash/monitor.

## Shared vs Local Files

- Committed:
  - `sdkconfig.defaults`
  - `.vscode/settings.defaults.json`
  - `.devcontainer/*`
- Local only (ignored):
  - `sdkconfig`
  - `.vscode/settings.json`

## First-Time Pull on Any Machine

After cloning or pulling:

```bash
cp .vscode/settings.defaults.json .vscode/settings.json
idf.py reconfigure
```

Then set your own serial port locally in VS Code settings.
