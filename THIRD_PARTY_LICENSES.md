# 第三方组件与许可声明

本仓库（ChrBase）自带若干**第三方**运行时库及其二进制。这些组件不是本项目的作品，
各自按其自身许可分发。本项目与它们之间是 **DLL 动态链接**关系 ——
因此本项目**自身的源代码不因这些许可而被覆盖**（不会被 LGPL 传染）。

> 本文档是合规性声明，不是法律意见。若要用于商业场景，建议自行复核。

---

## 1. FluidSynth

| 项 | 内容 |
|---|---|
| 组件 | FluidSynth（基于 SoundFont 2 规范的实时软件合成器）|
| 版本 | **2.6.0**（依据：随附头文件 `libs/fluidsynth/version.h` 的 `FLUIDSYNTH_VERSION "2.6.0"`）|
| 许可 | **GNU Lesser General Public License v2.1 or later**（SPDX：`LGPL-2.1-or-later`）|
| 用途 | 本项目的声卡后端（`VoiceCarDev`）|
| **来源** | FluidSynth 官方 Windows 发行包 **`fluidsynth-v2.6.0-win10-x64-cpp11`** 的 `bin/` 与 `include/` |
| 仓库内位置 | `libfluidsynth-3.dll`、`libs/libfluidsynth-3.lib`、`libs/fluidsynth.h`、`libs/fluidsynth/*.h` |
| 溯源校验 | `libfluidsynth-3.dll` 与发行包 `bin/libfluidsynth-3.dll` 的 SHA-256 一致（`635cb6dec5d92dd9…`）|
| 上游源码 | https://github.com/FluidSynth/fluidsynth —— tag **`v2.6.0`** |
| 许可全文 | [`licenses/LGPL-2.1.txt`](licenses/LGPL-2.1.txt) |

> ⚠️ **上游的坑**：FluidSynth 这个 Windows 发行包里**一份许可文件都没有**
> （既无 LICENSE 也无 COPYING/AUTHORS）。所以 `licenses/LGPL-2.1.txt` 是本项目
> 从 https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt 自行取得的官方原文。

## 2. libsndfile

| 项 | 内容 |
|---|---|
| 组件 | libsndfile（音频文件读写库）|
| 版本 | **1.2.2**（依据：DLL 内嵌版本串）|
| 许可 | **LGPL-2.1-or-later**（SPDX）|
| 用途 | FluidSynth 的依赖，用于声音文件读写 |
| **来源** | **随 FluidSynth 的 Windows 发行包一起提供**（`bin/sndfile.dll`）—— 不是单独下载的 |
| 仓库内位置 | `sndfile.dll` |
| 溯源校验 | 与 FluidSynth 发行包 `bin/sndfile.dll` 的 SHA-256 一致（`4e3bd2de8e148511…`）|
| 上游源码 | https://github.com/libsndfile/libsndfile —— tag **`1.2.2`** |
| 许可全文 | [`licenses/LGPL-2.1.txt`](licenses/LGPL-2.1.txt) |

> ⚠️ **注意**：libsndfile 的官方发行包通常还捆绑了若干第三方编解码库
> （FLAC / Ogg Vorbis / Opus / mpg123 / LAME 等），它们各有自己的许可。
> 而这个 `sndfile.dll` 是**从 FluidSynth 包里带出来的**，上游同样没有附任何许可或 AUTHORS，
> 所以**无法从本仓库内确证**它链接了哪些编解码库。
> 若你要**再分发** `sndfile.dll`，建议向 FluidSynth 项目确认其构建配置，
> 或改用 libsndfile 官方发布的 Windows 包（那里面有 `COPYING` / `AUTHORS`）。

## 3. SDL3

| 项 | 内容 |
|---|---|
| 组件 | Simple DirectMedia Layer 3 |
| 许可 | **zlib License**（宽松许可，非 copyleft）|
| 用途 | 窗口与图形输出 |
| 上游源码 | https://github.com/libsdl-org/SDL |
| 许可全文 | [`licenses/SDL3.txt`](licenses/SDL3.txt)（取自 SDL3-devel 发行包）|

**本仓库里其实有两份 SDL3，是不同版本、不同来源 —— 不是重复文件：**

| 位置 | 版本 | 来源 | 大小 |
|---|---|---|---|
| `SDL3.dll`（根目录）| **3.4.14** | `SDL3-devel-3.4.14-mingw` 的 `x86_64-w64-mingw32/bin/` | 5,381,521 字节 |
| `others/SDL3_from_FluidSynthLib/SDL3.dll` | **3.2.10** | **FluidSynth 2.6.0 发行包自带**的 `bin/` | 2,453,504 字节 |
| `libs/SDL3/*.h`、`libs/libSDL3.dll.a` | 3.4.14 | SDL3-devel 包的 `include/` 与 `lib/` | — |

> 溯源：根目录那份与 SDL3-devel 包的 x86_64 构建 SHA-256 一致（`4c2c02e0791f39fc…`）；
> `others/` 那份与 FluidSynth 包的 `bin/SDL3.dll` 一致（`2dfe1c60557c3e05…`）。
>
> ⚠️ **注意版本差异**：`others/` 里那份是 **3.2.10**，比根目录的 3.4.14 旧。
> 程序运行时按 DLL 搜索顺序只会用到一个；如果 `others/` 那份被放到 exe 旁边，
> 就会实际加载较旧的 SDL3。若留着只当参考，建议在文件名或说明里标明版本。

---

# LGPL-2.1 的合规要点

FluidSynth 与 libsndfile 都以 **LGPL-2.1** 分发。本项目采用的合规方式是：

| # | LGPL 的要求 | 本项目的做法 |
|---|---|---|
| 1 | **醒目声明**：声明使用了该库、受 LGPL 覆盖、**不提供任何担保** | 就是本文档 |
| 2 | **附上许可全文** | [`licenses/LGPL-2.1.txt`](licenses/LGPL-2.1.txt) |
| 3 | **允许使用者替换该库**（LGPL-2.1 §6b）| 采用 **DLL 动态链接** —— 使用者可以直接把 `libfluidsynth-3.dll` / `sndfile.dll` 换成自己改过的版本，**无需重新编译本项目** |
| 4 | **提供库源码的获取途径** | 上表已给出上游仓库与**确切版本号（tag）** |

## 结论：本项目自身代码不受 LGPL 约束

因为是**动态链接**而非静态链接：

- ✅ 本项目**不需要**因为用了 FluidSynth / libsndfile 而开源；
- ✅ 本项目可以自由选择自己的许可（MIT / Apache-2.0 / GPL / 保留所有权利 都可以）；
- ⚠️ 只有一种情况要注意：**如果你修改了 FluidSynth 或 libsndfile 本身的源码**，
  那被修改的那部分库必须仍按 LGPL 分发 —— 但这不影响你的项目代码。

---

# 本项目自身的许可

本项目（ChrBase）**自身的源代码**采用 **MIT 许可** —— 见仓库根目录的 [`LICENSE`](LICENSE)。
你可以自由使用、修改、再分发、闭源商用，只需保留版权声明与许可全文。

# 不在此列的组件

以下都是**本项目自身的作品**，不是第三方组件，适用上面的 MIT 许可：

- `main.cpp`、`_4refactoring/**`、`plans/**`、`test/**` —— 项目自身源码
- `libs/CKhash.h`、`libs/libCKhash.a`、`others/CKhash/*` —— 作者自研的哈希库
