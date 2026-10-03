# Astraea

Astraea is a Qt/C++ desktop shell for the flight simulation ecosystem. It provides a clean host for local tools, charts, flight data, and third-party extensions.

## Features

- Qt Widgets and C++17
- Windows, Linux, and macOS oriented project structure
- Segoe UI interface styling
- Qt/C++ plugin ABI in `src/plugins/plugininterface.h`
- Automatic discovery of native plugins and external plugin entries
- Plugin-driven sidebar navigation
- Optional Qlementine integration
- Windows packaging with Qt runtime deployment through `windeployqt`

## Build

Requirements:

- Qt 5.15+ or Qt 6 with Widgets and Network
- CMake 3.21+
- A C++17 compiler

```bash
cmake -S . -B build -DASTRAEA_FETCH_QLEMENTINE=ON
cmake --build build --config Release
```

When Qlementine is not available, set `ASTRAEA_FETCH_QLEMENTINE=OFF`. Astraea will use its built-in dark desktop theme.

On Windows, enable Qt deployment to place the Qt runtime beside the executable:

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_CXX_COMPILER="I:/Qt/Tools/mingw1310_64/bin/g++.exe" `
  -DCMAKE_PREFIX_PATH="I:/Qt/6.11.2/mingw_64" `
  -DASTRAEA_DEPLOY_QT=ON
cmake --build build --config Release
```

## Plugin directory

Place plugins in the `plugins/` directory next to `astraea.exe`. Use the Settings page to rescan the directory. Installed plugins are shown in the sidebar.

## Plugin types

### Native Qt/C++ plugin

Native plugins can be built as `.dll`, `.so`, or `.dylib` files using the interface in `src/plugins/plugininterface.h`.

### Manifest plugin

The host also supports `*.astraea.json` manifests:

```json
{
  "id": "charts.example",
  "name": "Charts Example",
  "description": "A web-based chart service",
  "type": "web",
  "entry": "https://example.com/"
}
```

Supported manifest types include `exe`, `app`, `command`, `web`, `url`, `html`, `page`, `folder`, and `script`.

Manifest entries may use relative paths inside a plugin folder. Executable and script plugins support `arguments`; script plugins also require an `interpreter` field.

### Direct plugin files

The host can also discover these files without a manifest:

- `.dll`, `.so`, `.dylib`
- `.exe`, `.com`, `.bat`, `.cmd`
- `.html`, `.htm`, `.url`

Only run plugins from trusted sources. External applications and scripts run with the current user permissions.

## Runtime output

The Windows output directory contains:

```text
build/bin/
├─ astraea.exe
├─ Qt6Core.dll
├─ Qt6Gui.dll
├─ Qt6Widgets.dll
├─ platforms/
├─ styles/
├─ imageformats/
└─ plugins/
```
