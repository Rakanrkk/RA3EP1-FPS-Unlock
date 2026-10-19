# RA3EP1 FPS Unlock

《命令与征服：红色警戒 3：起义时刻》（游戏程序 `ra3ep1_1.0.game`）的高帧率时序补丁。每档载荷把客户端更新频率与引擎固定的 15 Hz 逻辑时钟解耦，并针对该档位修正特效、粒子和悬挂的计时。English version: [README.md](README.md)。

本项目为非官方兼容项目，与 EA 无关联、未获背书。组件来源与许可见 [NOTICE.md](NOTICE.md)、[THIRD-PARTY.md](THIRD-PARTY.md) 与 [docs/provenance.md](docs/provenance.md)。

本项目独立于 macOS 移植版。这些补丁至今的真实对局验证环境是 Apple Silicon Mac（Wine/WoW64 + mtld3d），通过另行开发的 [RA3EP1Mac](https://github.com/Rakanrkk/RA3EP1Mac) 项目完成；该 Mac 封装在此仅作为已测试环境的记录，不是本项目的应用示例。

## 工作方式

起义时刻引擎以固定 15 Hz 周期模拟游戏逻辑，原版游戏把客户端更新与该时钟绑定。每档载荷把两者解耦：画面按所选频率更新，而 15 Hz 逻辑调度、原版 30 Hz 粒子模拟和原版 30 Hz 悬挂物理保持原有节奏；画面侧通过插值衔接更高刷新率。任何选项都不会改变游戏速度。各档调度细节见 [docs/architecture.md](docs/architecture.md)。

## 各档状态

| 档位 | 状态 |
| --- | --- |
| 60 FPS | 稳定档；长期使用基准，实机验证 |
| 120 FPS | 稳定档；实机测试，本机验收 |
| 30 FPS | 同一源码树的研究档位；原生引擎计时，仅计数插桩 |
| 90 FPS | 研究档位；已实机测试，见下方已知问题 |

**已知问题**：天狗（喷气形态）在移动中降落时的变形衔接在高档位下不能总是完整过渡，跳变问题仍在调查中。当前测试中原地降落不受影响。已测试并排除的方向记录在 [docs/research/90fps-landing.md](docs/research/90fps-landing.md)。

## 下载与使用

发行附件：`RA3EP1-FPS-Unlock-v1.0.0-windows-x86.zip`（32 位 Windows 载荷），附 `SHA256SUMS` 供核对。内含 `fps60.dll`、`fps120.dll`、加载器 `RA3EP1FPSUnlock.exe`、字节相同的 `Uprising60.exe` 副本（兼容既有工具）、`LICENSE` 与 `SHA256SUMS`。

加载器用法：

```text
RA3EP1FPSUnlock.exe GAME_DIRECTORY DLL_PATH
```

- `GAME_DIRECTORY` 是游戏根目录。加载器按 SHA-256 校验其中的 `Data/ra3ep1_1.0.game`，拒绝启动任何其他可执行文件，且从不写入游戏程序本身。
- 载荷诊断写入已加载 DLL 旁边的 `uprising120.log`。将 `U60_BASELINE` 设为非空值可保持原生计时，仅记录基线测量。

## 从源码构建

一份源码为每个目标档位构建一个载荷；档位是编译期定义，没有运行时切换。稳定发行构建 60 和 120；30 和 90 保留为同一源码的研究档位。

需要 CMake 3.15 或更新（推荐 Ninja）、Python 3，以及用于载荷的 i686 Windows 编译器（MinGW-w64 GCC 或 LLVM-MinGW Clang）。

主机测试可在任何有 C 编译器的平台运行：

```bash
cmake -S . -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

载荷与加载器（i686 Windows，交叉或本机）：

```bash
cmake -S . -B build/i686 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/i686-windows.cmake \
  -DRA3EP1FPSUNLOCK_C_COMPILER=/path/to/i686-w64-mingw32-clang \
  -DBUILD_TESTING=OFF
cmake --build build/i686
python3 tools/release.py --build-dir build/i686 \
  --output dist/RA3EP1-FPS-Unlock-v1.0.0-windows-x86.zip
```

完整工具与选项参考：[docs/usage.md](docs/usage.md)。

## 测试

验证分阶段进行：各档位调度与时钟数学的主机测试套件、独立的行为契约检查（[`tests/contract_check.py`](tests/contract_check.py)）、真实 32 位桥接夹具，以及带逐窗口遥测的真实对局。每个阶段证明了什么、刻意不证明什么，见 [docs/testing.md](docs/testing.md)。

## 文档

- [docs/architecture.md](docs/architecture.md) — 计时模型与各档调度
- [docs/usage.md](docs/usage.md) — 构建、发行与加载器参考
- [docs/testing.md](docs/testing.md) — 测试范围与边界
- [docs/methods.md](docs/methods.md) — 一手分析与验证方法
- [docs/research/90fps-landing.md](docs/research/90fps-landing.md) — 90 FPS 降落问题调查
- [docs/provenance.md](docs/provenance.md) — 来源、参考与许可

## 相关项目

- [RA3EP1Mac](https://github.com/Rakanrkk/RA3EP1Mac) — 独立的 macOS 兼容项目；Windows 版游戏在 Apple Silicon（Wine）环境配合这些补丁实际测试的场所。

## 许可

本仓库的原创源码、工具和文档以 [MIT 许可](LICENSE)发布；范围与第三方参考见 [NOTICE.md](NOTICE.md)。游戏及其资源为专有内容，不在本仓库中。与 Wine 相关的代码和许可在 RA3EP1Mac 项目内；本仓库不含 Wine 代码。
