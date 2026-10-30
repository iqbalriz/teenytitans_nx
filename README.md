<div align="center">

<img src="launcher/icon.jpg" alt="Teeny Titans" width="160">

# teenytitans_nx

**Teeny Titans: Teen Titans GO! for Nintendo Switch**

An unofficial Nintendo Switch native wrapper for the 32-bit Android release of  
**Teeny Titans: Teen Titans GO!** (Grumpyface Studios / Cartoon Network).

[![Nintendo Switch](https://img.shields.io/badge/Nintendo_Switch-Homebrew-E60012?style=for-the-badge&logo=nintendoswitch&logoColor=white)](#)
[![Version](https://img.shields.io/badge/Version-0.1.0-4C8BF5?style=for-the-badge)](#)
[![Architecture](https://img.shields.io/badge/AArch32-32--bit_Native-6A1B9A?style=for-the-badge)](#)
[![Unity](https://img.shields.io/badge/Unity-2017.4_IL2CPP-222222?style=for-the-badge&logo=unity&logoColor=white)](#)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg?style=for-the-badge)](LICENSE)

</div>

---

## About

`teenytitans_nx` is a native wrapper that runs the 32-bit ARM Android build of **Teeny Titans 1.2.7** (`com.turner.ttgfigures`) on the Nintendo Switch, in TATE mode: the game is portrait, so you hold the console upright.

It loads the game's original Unity 2017.4 libraries (`libmain.so`, `libunity.so` and `libil2cpp.so`) and plays the part of the game's Java side: the activity, the audio output, the input events and the plugins the game calls.

It is built on [aks796](https://github.com/aks796)'s [`dcr_sea_nx`](https://github.com/aks796/dcr_sea_nx) (Disney Crossy Road), through the `stevenrpg_nx` port, with the Mono parts replaced by what IL2CPP needs: a stop-the-world bridge for its Boehm garbage collector, since Horizon has no signals.

Because the Tegra X1 CPU in the Nintendo Switch natively supports 32-bit ARM (AArch32) execution, the game code runs **directly on the hardware**, with no emulation.

> [!NOTE]
> **No game code or assets are included in this repository or in the release.**  
> Users must supply their own legitimate copy of the game's APK and expansion file (OBB).

> [!IMPORTANT]
> The port is made for version **1.2.7** (version code 1151). Its garbage collector bridge and its movie hooks use addresses of that build. With another version they are refused at start-up and the game will not run properly.

---

## Features

- **TATE Mode:** The picture is turned for a console held upright, in the direction you choose. Touch, sticks and the D-pad turn with it.
- **Full Native Performance:** Runs natively on the Switch ARM Cortex-A57 CPU in AArch32 mode.
- **Hardware-Accelerated Graphics:** OpenGL ES via `mesa32` (Nouveau), at 720x1280 in handheld.
- **Touchscreen & Controller Support:** The game is made for touch only. Touch works as on mobile; a controller plays it as a finger, with a cursor and swipes.
- **Sound:** The game's music and effects (FMOD) played through the Switch's audio output.
- **Movies:** The logos and the intro play (H.264, decoded by FFmpeg, LGPL). The wide movies fill the screen with the console held normally. A button or a touch skips them.
- **Atmosphère & Sphaira Integration:** A dedicated launcher NRO registers a HOME menu forwarder icon.

---

## Requirements

### For Players
- A Nintendo Switch running **Atmosphère** custom firmware.
- The [Sphaira](https://github.com/ITotalJustice/sphaira) homebrew menu (Important: you need to install Sphaira's forwarder, otherwise the port will not start).
- A copy of **Teeny Titans 1.2.7** for Android (`com.turner.ttgfigures`), the APK with `lib/armeabi-v7a/` inside.
- Its expansion file, **`main.1151.com.turner.ttgfigures.obb`** (211 MB; on a phone it is in `Android/obb/com.turner.ttgfigures/`). The APK does not work without it.

---

## Installation Guide

1. Download the latest release from the [Releases](../../releases) tab:
   - `teenytitans_nx_0.1.0.zip`
2. Copy the `switch/teenytitans_nx/` folder from the ZIP to the root of your SD card, so you get:
   ```text
   sdmc:/switch/teenytitans_nx/
   ```
3. Place your APK and your OBB inside that folder:
   - `teenytitans_nx.nro` is already there (from the ZIP).
   - Copy your APK into `sdmc:/switch/teenytitans_nx/` (the name of the APK does not matter).
   - Copy your `main.1151.com.turner.ttgfigures.obb` into `sdmc:/switch/teenytitans_nx/obb/`.
4. The final folder structure on your SD card must look like:
   ```text
   sdmc:/switch/teenytitans_nx/
   ├── teenytitans_nx.nro
   ├── your-game.apk
   └── obb/
       └── main.1151.com.turner.ttgfigures.obb
   ```
5. Launch **Sphaira** on your Switch:
   - Navigate to **Homebrew** › **Teeny Titans**.
   - Choose **Install Forwarder**.
   - Return to the Switch HOME Menu and launch the game directly from its icon!

The first start rewrites the APK without compression and unpacks the game's libraries from it (a minute or two, with a progress bar; again only when the APK changes).

---

## Controls

Hold the console upright, with its right side up (or its left side: see `tate_rotation`). Directions below are as you see them.

| Input | Action |
| :--- | :--- |
| **Touchscreen** | Direct touch controls (identical to the mobile version) |
| **Right Stick** | Show and move the **cursor** (it fades a few seconds after you stop) |
| **A / ZR** | Tap with the cursor; hold it to drag |
| **Left Stick / D-Pad** | Swipe |
| **L / R** | Swipe a page left / right |
| **B / − (Minus)** | Back / pause (Android's Back key) |
| **+ (Plus)** | Nothing |
| **During a movie** | Any button or a touch skips it |

---

## Configuration

`config.ini` is written to `sdmc:/switch/teenytitans_nx/` on the first start. Changes apply the next time the game starts.

<details>
<summary>All options</summary>

| Section | Option | Values |
| :--- | :--- | :--- |
| `[display]` | `resolution` | `auto` (default: 1080 docked, 720 handheld), `720`, `1080` |
| `[display]` | `tate_rotation` | `right` (default: the console's right side up), `left` |
| `[performance]` | `boost_cpu_when_loading` | CPU at 1785 MHz while the game starts and in loading frames |
| `[audio]` | `mix_at_48khz` | `true` / `false` |
| `[debug]` | `gl_selftest`, `profile_long_frames`, `boot_log_on_screen`, `log_sounds` | for bug reports |

</details>

If something goes wrong, `debug.log` and `crash.log` are written next to `config.ini`. Send them with a bug report.

---

## Not in This Port

The store and purchases, ads, analytics and the online features are switched off: the game's Java side answers as a phone with no network.

Docked play works, but the picture is turned on the TV too: the port is meant for handheld.

---

## Building from Source

### Prerequisites
- Linux (Ubuntu / Debian / Linux Mint recommended), or Windows with PowerShell
- **Docker**
- Git

> [!IMPORTANT]
> **On Windows, run `git config --global core.autocrlf false` before cloning.** With `autocrlf=true` the runtime's scripts and Makefiles are checked out with CRLF line endings and break in Docker.

### Build Instructions

1. **Clone the repository with submodules:**
   ```bash
   git clone --recursive https://github.com/iqbalriz/teenytitans_nx.git
   cd teenytitans_nx
   ```

2. **Pull the required Docker toolchains:**
   ```bash
   docker pull ghcr.io/vita2hos/devcontainer/vita2hos
   docker pull devkitpro/devkita64
   ```

3. **Set up `libnx32`, `mesa32` and `ffmpeg32`:**
   - Compile `libnx32` next to this folder (the build looks for `../libnx32/prefix`):
     ```bash
     git clone https://github.com/aks796/libnx32.git ../libnx32
     ../libnx32/build.sh
     ```
   - Download the prebuilt `mesa32` release into `portlibs32/`:
     ```bash
     tools/get_portlibs.sh
     ```
     (On Windows: `tools\get_portlibs.ps1`.)
   - Build `ffmpeg32` **with the H.264 decoder** (the movies; LGPL) and copy its `lib/` and `include/` into `portlibs32/`:
     ```bash
     git clone https://github.com/aks796/ffmpeg32.git ../ffmpeg32
     FFMPEG_COMPONENTS="--enable-demuxer=mov --enable-decoder=h264 --enable-parser=h264 \
       --enable-decoder=aac --enable-parser=aac" ../ffmpeg32/build.sh
     cp -rf ../ffmpeg32/prefix/lib ../ffmpeg32/prefix/include portlibs32/
     ```
     Without it the port builds, but the game cannot pass its intro.

4. **Compile the 32-bit program:**
   ```bash
   ./build.sh
   ```
   (On Windows: `.\build.ps1`.) This makes `teenytitans_nx.nsp`.

5. **Compile the 64-bit launcher NRO:**
   ```bash
   launcher/build.sh
   ```
   The compiled launcher will be located at `launcher/teenytitans_nx.nro`.

The engine offsets in `source/dcr_offsets.h` and the import table in `source/imports.c` come from the game's own libraries: `tools/offsets/derive_offsets.py` and `runtime/tools/gen_imports.py`. `tools/il2cpp_methods.py` lists the game's C# methods with their addresses, from `global-metadata.dat` and `libil2cpp.so`.

---

## Credits & Acknowledgments

- **Grumpyface Studios & Cartoon Network**: Original creators of Teeny Titans. Teen Titans GO! is a trademark of DC Comics and Warner Bros. Entertainment.
- **[aks796](https://github.com/aks796)**: For [`dcr_sea_nx`](https://github.com/aks796/dcr_sea_nx), the Unity port this one is built on, and for the `android32`, `libnx32`, `mesa32` and `ffmpeg32` projects.
- **FFmpeg**: The MP4 demuxer and the H.264 and AAC decoders that play the movies (LGPL v2.1+, linked statically; source at ffmpeg.org).
- **Andy Nguyen (TheOfficialFloW) & fgsfds**: Dynamic `.so` loader implementations.
- **xerpi**: For `vita2hos`, pioneer of AArch32 native execution on Switch.
- **Switchbrew**: For `libnx` and tools.
- **ITotalJustice**: For Sphaira.

---

## License

This project is licensed under the [MIT License](LICENSE).
Teen Titans GO! is a trademark of DC Comics and Warner Bros. Entertainment; the game is by Grumpyface Studios. This project is not affiliated with or endorsed by Cartoon Network, Warner Bros. or Nintendo.
