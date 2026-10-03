# Astraea

Astraea 是一个面向模拟飞行生态的 Qt/C++ 插件宿主。它的目标是用一个统一的桌面外壳承载本地工具、飞行资料、地图与第三方插件。

## 特性

- Qt Widgets + C++17，Windows/Linux/macOS 友好
- 中英文界面切换并保存语言偏好，默认使用 Segoe UI 字体
- 稳定的 C++ 插件 ABI 入口：`src/plugins/plugininterface.h`
- 从 `plugins/` 目录动态发现 DLL/SO/DYLIB、EXE、Web、HTML 与脚本插件
- 插件驱动的左侧导航：仅显示用户实际安装的插件
- 可选接入 [Qlementine](https://github.com/oclero/qlementine)

## 构建

需要 Qt 5.15+ 或 Qt 6（Widgets、Network）与 CMake 3.21+：

```bash
cmake -S . -B build -DASTRAEA_FETCH_QLEMENTINE=ON
cmake --build build --config Release
```

不想在线获取 Qlementine 时，将选项设为 `OFF`；Astraea 会使用内置的 Qlementine-inspired 深色主题，接口与布局保持不变。

插件编译完成后放入运行目录下的 `plugins/`，应用会在启动时加载它们。
默认不附带插件或航图入口。添加插件后，可以在设置页点击“重新扫描插件”，左侧会出现相应入口。侧边栏支持收起和展开。

## 插件类型

除了 Qt/C++ 原生插件，插件目录还支持 `*.astraea.json` 清单：

```json
{
  "id": "simbrief.web",
  "name": "SimBrief",
  "description": "Flight planning service",
  "type": "web",
  "entry": "https://www.simbrief.com/"
}
```

`type` 可使用 `exe`、`app`、`command`、`web`、`url`、`html`、`page`、`folder`、`script`。同时可以直接放入 `.exe`、`.bat`、`.cmd`、`.html`、`.htm` 或 `.url` 文件，宿主会自动识别。EXE/脚本支持 `arguments`，脚本另外需要 `interpreter`。HTML、EXE、脚本可以放在插件子目录中，并通过相对路径引用。

## Windows 发布

项目提供 `ASTRAEA_DEPLOY_QT` 选项。开启后会在构建结束时调用 Qt 官方 `windeployqt`，把 Qt DLL、平台插件和依赖复制到 `build/bin`，因此目标机器不需要安装 Qt：

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_CXX_COMPILER="I:/Qt/Tools/mingw1310_64/bin/g++.exe" `
  -DCMAKE_PREFIX_PATH="I:/Qt/6.11.2/mingw_64" `
  -DASTRAEA_DEPLOY_QT=ON
cmake --build build --config Release
```
