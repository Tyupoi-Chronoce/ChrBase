# ChrBase

**English** ｜ [简体中文](README.zh-CN.md)

A from-scratch **32-bit virtual CPU / instruction-set simulator** in C++17 — segmented memory
with PID-based access protection, an x86-like instruction set, a multi-core *processing center*,
a vector *graphics center*, and a device framework.

> ⚠️ **Work in progress.** `main.cpp` is ~4,600 lines and still growing. The emulator boots,
> runs its fetch–decode–execute loop and prints a live debug HUD — but several major
> subsystems (jumps, I/O, disk, boot) are **not implemented yet**. See [Status](#status).

Licensed under the [MIT License](LICENSE).
Bundled third-party libraries are covered separately in [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md).

---

## Status

Current version: **`0.1.9R-1`**

**Working**

| Subsystem | Notes |
|---|---|
| `Ram` | Segmented memory (256 KiB/segment), per-segment PID locks (`rootPID` / `constPID` / `publicPID`), unified `resData` return separating payload from UAF/MOF flags |
| `reg32` / `reg16` / `regF32` | Register file, with `HalfRef` 16-bit views |
| ALU | `mov xchg cbw cwd add sub adc sbb and or xor inc dec cmp test mul imul div idiv`, 32/16-bit overloads |
| Stack | `push pop pushf popf pushSeg popSeg` |
| Decoder | 80 opcodes (`0x00`–`0x5F`), plus combined opcodes `0xFF` / `0xFE` |
| `debugCMD` | Live register / flag HUD, refreshes in a separate thread |
| `Center32x` | Main processing center — multi-core, mutex-protected fetch |
| CKhash | Vendor-neutral device IDs via a custom hash library |

**Partial**

- Device framework: `devIO` is now an abstract base (`virtual void loop() = 0`), with 7 device
  classes deriving from it — but only `GraphicDev` implements `loop()` yet, so the other six are
  still abstract and cannot be instantiated.
- `_4refactoring/` — `Ram` and `Regs` are already packaged as static libs (`libRam.a`,
  `libRegs.a`); `Center32g` (the graphics center) has its own decoder and `addressing()`.
  **None of it is wired into the build yet** — `main.cpp` doesn't reference it.

**Not implemented**

- **Jumps — the entire family.** `jmp` / `jcc` / `call` / `ret` / `iret` appear **zero times** in
  `main.cpp`. This is the #1 blocker for the long-term goal of self-hosting.
- Shifts & rotates (`shl shr sal sar rol ror`), `not`, `neg`
- `in` / `out` and an actual I/O protocol
- `DiskDev` — currently just holds a path and a JSON filename; no file handle, no sector reads
- Configuration parsing — the project reads no JSON or external file at all yet
- `rootOP` — still an empty class
- Boot flow — copying a bootable image into virtual memory and starting from it
- No SDL window yet (`SDL_` is called zero times)

## What it looks like right now

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

## Repository layout

```
main.cpp                  the emulator — ~4,600 lines, everything currently runs from here
version.global            the version number to be used for the NEXT commit/push
LICENSE                   MIT
THIRD_PARTY_LICENSES.md   bundled libraries and their licenses
licenses/                 full license texts (LGPL-2.1, SDL3/zlib)

libs/                     third-party headers + import libs + our own CKhash
  fluidsynth.h, fluidsynth/*.h    FluidSynth 2.6.0 API
  SDL3/*.h, SDL.h                 SDL3 3.4.14 API
  CKhash.h, libCKhash.a           custom hash library

_4refactoring/            modularized rewrite, not yet wired into the build
  Ram/, register/                  originals + *_libF.cpp library-formatted copies
  _libs/                           Ram.h / Regs.h + libRam.a / libRegs.a
  Center/graphCenters.cpp          Center32g — vector ALU array (graphics center)
  Device/device_basiClass.cpp      Screen + devIO abstract base

others/                   CKhash sources, a reference SDL3 build, old experiments
plans/                    design notes and versioned roadmaps
test/                     small standalone experiments (not part of the build)

SDL3.dll                  bundled runtime DLLs — required next to the executable
libfluidsynth-3.dll
sndfile.dll
```

## Build

Requires **MinGW-w64** — developed against [w64devkit](https://github.com/skeeto/w64devkit)
with `g++` 14.2. No CMake, no Makefile; the build is a single command (also wired up as a
VS Code task in `.vscode/tasks.json`):

```bash
g++ -Ilibs -Llibs main.cpp -lfluidsynth-3 -lSDL3 -o main.exe
```

Then put the three DLLs (`SDL3.dll`, `libfluidsynth-3.dll`, `sndfile.dll`) **next to** the
executable and run it.

> Verified on 2026-10-13: builds clean in ~7 s with the exact command above.

## What you need to supply yourself

A few resources are intentionally **not** in this repository (licensing, mostly):

- **SoundFont for audio.** The audio backend (FluidSynth) needs an `.sf2` file. Create an
  `audio_sources/` directory and drop one in. Nothing in the code hard-codes a SoundFont path
  yet, so you choose the name — see `VoiceCarDev` in `main.cpp`.
- **Fonts**, if you intend to use the text-rendering path — `fonts/` currently only holds a
  placeholder.

## Roadmap

Taken from `plans/`:

| Plan | Goal |
|---|---|
| `v0.1.11T` | Refactor for flexibility and maintainability |
| `v0.1.14T` | I/O devices registered in the centers' registry; a dedicated class to process I/O data |
| `v0.1.28T` | `rootOP` controlling all processing centers with unique PIDs; I/O control and data transfer; stack handling; a JSON file deciding which disks to load and which to boot from |
| `v0.1.52T` | Interactive debug console (`int` / `resume` / `kill` / `quit` / `step` / `next`) |

The long-term goal is a **self-hosting** virtual machine: read device parameters from a
`BIOS.json`-style config, copy instructions from a bootable virtual disk image into virtual
memory, and then run and branch on its own.

## License

Project code: **MIT** — see [LICENSE](LICENSE).

Bundled third-party libraries are distributed under their own licenses
(FluidSynth and libsndfile are LGPL-2.1; SDL3 is zlib). They are linked **dynamically**,
so this project's own source is not affected by their terms.
Details: [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md).
