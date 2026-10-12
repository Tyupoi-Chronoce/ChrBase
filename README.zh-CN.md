# ChrBase

[English](README.md) ｜ **简体中文**

一个从零手写的 **32 位虚拟 CPU / 指令集模拟器**（C++17）——分段内存 + 基于 PID 的访问保护、
类 x86 指令集、多核「处理中心」、向量式「图像中心」，以及一套设备框架。

> ⚠️ **开发中。** `main.cpp` 约 4,600 行，还在持续增长。模拟器目前能启动、跑取指—译码—执行循环、
> 并打印实时调试 HUD —— 但好几个主要子系统（跳转、I/O、磁盘、引导）**尚未实现**。
> 详见下方的[当前状态](#当前状态)。

本项目以 [MIT 许可](LICENSE) 开源。
随仓库分发的第三方库另有声明，见 [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md)。

---

## 当前状态

当前版本：**`0.1.9R-1`**

**已可用**

| 子系统 | 说明 |
|---|---|
| `Ram` | 分段内存（每段 256 KiB），逐段的 PID 锁（`rootPID` / `constPID` / `publicPID`），统一用 `resData` 返回、把数据与 UAF/MOF 标志分离 |
| `reg32` / `reg16` / `regF32` | 寄存器组，含 `HalfRef` 16 位视图 |
| ALU | `mov xchg cbw cwd add sub adc sbb and or xor inc dec cmp test mul imul div idiv`，含 32/16 位重载 |
| 栈 | `push pop pushf popf pushSeg popSeg` |
| 译码器 | 80 个操作码（`0x00`–`0x5F`），另有组合操作码 `0xFF` / `0xFE` |
| `debugCMD` | 实时寄存器 / 标志位 HUD，在独立线程刷新 |
| `Center32x` | 主处理中心 —— 多核，取指流程有互斥锁保护 |
| CKhash | 用自研哈希库生成与厂商无关的设备 ID |

**部分完成**

- **设备框架**：`devIO` 已升级为抽象基类（`virtual void loop() = 0`），有 7 个设备类继承它 ——
  但目前只有 `GraphicDev` 实现了 `loop()`，其余 6 个仍是抽象类、**无法实例化**。
- **`_4refactoring/`**：`Ram` 与 `Regs` 已打包成静态库（`libRam.a`、`libRegs.a`）；
  `Center32g`（图像中心）有了自己的译码器与 `addressing()`。
  **但这些都还没接进构建** —— `main.cpp` 目前完全没有引用它们。

**尚未实现**

- **跳转指令整个家族。** `jmp` / `jcc` / `call` / `ret` / `iret` 在 `main.cpp` 里**出现 0 次**。
  这是通往"自运行"的头号阻塞。
- 移位与循环移位（`shl shr sal sar rol ror`）、`not`、`neg`
- `in` / `out` 指令与真正的 I/O 协议
- `DiskDev` —— 目前只存了一个路径和一个 JSON 文件名，没有文件句柄、不能读扇区
- 配置解析 —— 全项目目前不读任何 JSON 或外部文件
- `rootOP` —— 仍是空类
- 引导流程 —— 把可引导镜像拷进虚拟内存并从那里开始执行
- 还没有 SDL 窗口（`SDL_` 调用数为 0）

## 它现在长什么样

```
$ ./main.exe
Version 0.1.9R-1, initialization complete, girls praying...
Center 00 ==----	PID : 0000		runTimes : 0000000000000001
	AX : 0000 abcd	CX : 0000 0000		SP : 3fffffff	BP : 3ff00000		CS : 00000000	DS : 00000000
	DX : 0000 0000	BX : 0000 0000		SI : 00000000	DI : 00000000		SS : 00000000	ES : 00000000
	IP : 00000003	FLAGS : 00000000000000000000000000000000			FS : 00000000	GS : 00000000
	FP : 3fe00001
----==end 00
Center 01 ==----	PID : 0000		runTimes : 0000000000000001
	...
```

## 目录导览

```
main.cpp                  模拟器本体 —— 约 4,600 行，目前所有逻辑都在这里
version.global            下次提交推送要用的版本号
LICENSE                   MIT
THIRD_PARTY_LICENSES.md   第三方组件及其许可
licenses/                 许可全文（LGPL-2.1、SDL3/zlib）

libs/                     第三方头文件 + 导入库 + 自研的 CKhash
  fluidsynth.h, fluidsynth/*.h    FluidSynth 2.6.0 API
  SDL3/*.h, SDL.h                 SDL3 3.4.14 API
  CKhash.h, libCKhash.a           自研哈希库

_4refactoring/            模块化重构，尚未接进构建
  Ram/, register/                 原件 + *_libF.cpp 的库格式副本
  _libs/                          Ram.h / Regs.h + libRam.a / libRegs.a
  Center/graphCenters.cpp         Center32g —— 向量式 ALU 阵列（图像中心）
  Device/device_basiClass.cpp     Screen + devIO 抽象基类

others/                   CKhash 源码、一份参考用的 SDL3 构建、早期实验
plans/                    设计笔记与按版本排列的路线图
test/                     独立的小实验（不属于构建）

SDL3.dll                  随仓库分发的运行时 DLL —— 需要放在可执行文件旁边
libfluidsynth-3.dll
sndfile.dll
```

## 构建

需要 **MinGW-w64** —— 开发用的是 [w64devkit](https://github.com/skeeto/w64devkit) +
`g++` 14.2。没有 CMake、没有 Makefile，构建就一条命令
（同时也配置成了 VS Code 任务，见 `.vscode/tasks.json`）：

```bash
g++ -Ilibs -Llibs main.cpp -lfluidsynth-3 -lSDL3 -o main.exe
```

然后把 `SDL3.dll`、`libfluidsynth-3.dll`、`sndfile.dll` 三个 DLL 放到可执行文件**同目录**再运行。

> 2026-10-13 实测：用上面这条命令可干净编译，约 7 秒。

## 需要自备的资源

有几样东西**故意没有**放进仓库（主要是许可原因）：

- **音频用的 SoundFont。** 声卡后端（FluidSynth）需要一个 `.sf2` 文件。
  自己建一个 `audio_sources/` 目录放进去即可。代码里目前没有硬编码任何 SoundFont 路径，
  文件名由你定 —— 相关逻辑见 `main.cpp` 里的 `VoiceCarDev`。
- **字体**（如果要走文字渲染那条路）—— `fonts/` 目前只放了一个占位文件。

## 路线图

摘自 `plans/`：

| 计划 | 目标 |
|---|---|
| `v0.1.11T` | 重构，提升灵活性与可维护性 |
| `v0.1.14T` | I/O 设备可注册到各中心的注册表；用一个专门的类处理 I/O 数据 |
| `v0.1.28T` | `rootOP` 统管所有处理中心、各中心 PID 唯一；用 I/O 命令控制设备并传数据；栈的处理；用一份 JSON 决定加载哪些磁盘、从哪块盘引导 |
| `v0.1.52T` | 交互式调试台（`int` / `resume` / `kill` / `quit` / `step` / `next`） |

长期目标是让虚拟机**自运行**：从 `BIOS.json` 这类配置文件读取设备参数，把可引导虚拟磁盘里的
指令拷进虚拟内存，然后自己执行运算与跳转。

## 许可

项目自身代码：**MIT** —— 见 [LICENSE](LICENSE)。

随仓库分发的第三方库各自按其许可分发（FluidSynth 与 libsndfile 是 LGPL-2.1，SDL3 是 zlib）。
本项目以**动态链接**方式使用它们，因此本项目自身的源码不受这些许可的约束。
详见 [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md)。
