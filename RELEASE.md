# RA3EP1 FPS Unlock 1.0.0

## English

RA3EP1 FPS Unlock 1.0.0 is the first release of the game timing patches for
Command & Conquer: Red Alert 3 – Uprising (`ra3ep1_1.0.game`).

### What it does

- Client update rates decoupled from the engine's fixed 15 Hz logic clock.
  The stable release covers 60 and 120 FPS; 30 and 90 remain research
  switches of the same source tree. No option changes simulation speed.
- Particles keep the authored 30 Hz simulation; suspension physics keeps the
  authored 30 Hz cadence while the display interpolates poses at the chosen
  rate. Effect time domains are corrected per rate.
- All changes are runtime-only: save layout, replay data and native physics
  state are untouched, so existing saves and replays keep working.
- One source tree builds one DLL per rate, and each release archive carries a
  `SHA256SUMS` file.

### Current status

- **60 FPS**: long-standing baseline, live-verified.
- **120 FPS**: live-tested and locally accepted (steady match windows
  measured approximately 120 client updates/s and 15 logic cycles/s).
- **90 FPS**: research. The Jet Tengu's transformation when landing while
  still moving does not always play through cleanly at this rate; the
  investigation continues — see
  [docs/research/90fps-landing.md](docs/research/90fps-landing.md).
  Stationary landings are unaffected in current tests.
- **30 FPS**: research; native engine timing with counter instrumentation.

### Tested environment

Real game sessions were run on macOS (Apple Silicon) through the independent
[RA3EP1Mac](https://github.com/Rakanrkk/RA3EP1Mac) project, which is the
environment recorded for these patches. Windows-native execution is not
asserted by this release; see [docs/testing.md](docs/testing.md).

### Download

- `RA3EP1-FPS-Unlock-v1.0.0-windows-x86.zip` — prebuilt 32-bit Windows
  payloads: `fps60.dll`, `fps120.dll`, the loader `RA3EP1FPSUnlock.exe` (an
  identical copy named `Uprising60.exe` for existing tooling), `LICENSE` and
  `SHA256SUMS`. Verify `SHA256SUMS` before use.
- Run: `RA3EP1FPSUnlock.exe GAME_DIRECTORY DLL_PATH` — requires the supported
  game executable (`Data/ra3ep1_1.0.game`); the loader refuses any other
  executable and never writes the game.
- Source is available from this repository and the attached source archive.

### Build

See the README build section and [docs/usage.md](docs/usage.md): CMake 3.15+,
Python 3, and an i686 Windows compiler (MinGW-w64 GCC or LLVM-MinGW Clang).

### License and provenance

Original source, tools and documentation: MIT. Third-party references and
scope: [NOTICE.md](NOTICE.md), [THIRD-PARTY.md](THIRD-PARTY.md) and
[docs/provenance.md](docs/provenance.md). The SAGE-Unlocked project is
referenced for search patterns only; its GPL-licensed code is not used. The
game and its assets are proprietary and are not part of this repository.

---

## 中文

RA3EP1 FPS Unlock 1.0.0 是《命令与征服：红色警戒 3：起义时刻》（`ra3ep1_1.0.game`）
游戏时序补丁的首个发行。

### 功能

- 客户端更新频率与引擎固定 15 Hz 逻辑时钟解耦。稳定发行覆盖 60 和 120 FPS；
  30 和 90 保留为同一源码树的研究档位。任何选项都不会改变游戏速度。
- 粒子保持原版 30 Hz 模拟；悬挂物理保持原版 30 Hz 节奏，画面按所选频率插值
  姿态。特效时间域按档位修正。
- 全部修改仅作用于运行时代码：存档布局、录像数据和原生物理状态不变，现有
  存档和录像继续可用。
- 一份源码为每档构建一个 DLL，发行归档附 `SHA256SUMS`。

### 当前状态

- **60 FPS**：长期使用基准，实机验证。
- **120 FPS**：实机测试并获本机验收（稳定对局窗口实测约 120 次客户端
  更新/秒、15 次逻辑周期/秒）。
- **90 FPS**：研究档位。天狗（喷气形态）在该档位移动中降落时的变形衔接不能
  总是完整过渡，调查仍在继续——见
  [docs/research/90fps-landing.md](docs/research/90fps-landing.md)。当前测试
  中原地降落不受影响。
- **30 FPS**：研究档位；原生引擎计时，仅计数插桩。

### 测试环境

真实对局在 macOS（Apple Silicon）上通过独立的
[RA3EP1Mac](https://github.com/Rakanrkk/RA3EP1Mac) 项目完成，该项目是这些
补丁的记录测试环境。本发行不声称 Windows 原生环境已实测；
见 [docs/testing.md](docs/testing.md)。

### 下载

- `RA3EP1-FPS-Unlock-v1.0.0-windows-x86.zip` — 预编译 32 位 Windows 载荷：
  `fps60.dll`、`fps120.dll`、加载器 `RA3EP1FPSUnlock.exe`（附字节相同的
  `Uprising60.exe` 副本兼容既有工具）、`LICENSE` 与 `SHA256SUMS`。使用前请
  核对 `SHA256SUMS`。
- 运行：`RA3EP1FPSUnlock.exe GAME_DIRECTORY DLL_PATH` — 需要受支持的游戏程序
  （`Data/ra3ep1_1.0.game`）；加载器拒绝任何其他可执行文件，且从不写入游戏。
- 源码可从本仓库获取，另附源码归档。

### 构建

见 README 构建一节与 [docs/usage.md](docs/usage.md)：CMake 3.15+、
Python 3，以及 i686 Windows 编译器（MinGW-w64 GCC 或 LLVM-MinGW Clang）。

### 许可与来源

原创源码、工具和文档：MIT。第三方参考与范围：[NOTICE.md](NOTICE.md)、
[THIRD-PARTY.md](THIRD-PARTY.md) 与 [docs/provenance.md](docs/provenance.md)。
SAGE-Unlocked 项目仅作为定位模式的参考；其 GPL 许可代码未被使用。游戏及其
资源为专有内容，不在本仓库中。
