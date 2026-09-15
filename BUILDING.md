# Building HyperTube Music

## Requirements

- CMake 3.28 or newer, and Ninja
- A C++20 compiler (GCC, Clang or MSVC)
- Qt 6.8 to 6.11 (6.11.2 recommended) with Qt Declarative, Svg, ShaderTools, WebEngine and the Linguist tools.
- libmpv
- Python 3 with `fonttools`
- libcurl development headers (Linux only, required for crash reporting)

The first configure downloads sentry-native, so it needs a network connection. To build without crash reporting and download nothing, pass `-DHT_MUSIC_CRASH_REPORTING=OFF`.

## Linux

On Arch Linux, install the dependencies with:

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-declarative qt6-svg \
    qt6-shadertools qt6-webengine qt6-tools mpv python-fonttools curl
```

On other distributions, install the equivalent packages. Build and run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
./build/app/ht-music
```

To install system-wide:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

## Windows

Build from a Visual Studio x64 Native Tools prompt with Qt (MSVC 2022 64-bit), CMake, Ninja and Python on `PATH`, and `fonttools` installed through pip.

libmpv comes from a [shinchiro build](https://github.com/shinchiro/mpv-winbuild-cmake/releases) (`mpv-dev-x86_64-*.7z`). That package ships only a MinGW import library, so generate `lib\mpv.lib` from `libmpv-2.dll` with `dumpbin /exports` and `lib /def:`. Then point `MPV_ROOT` at the extracted folder:

```bat
set MPV_ROOT=C:\dev\deps\mpv
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/msvc2022_64
cmake --build build
```

`build\ht-music.exe` needs Qt and libmpv on `PATH` to run. Two optional targets package it:

```bat
cmake --build build --target ht-music-stage
cmake --build build --target ht-music-installer
```

The first copies every runtime dependency next to the executable in `build\stage\app`. The second builds `build\HyperTubeMusic-Setup-<version>-x64.exe`.

## macOS

Install Qt with the official installer (including the WebEngine module), then:

```bash
brew install cmake ninja mpv
python3 -m pip install fonttools
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH=~/Qt/6.11.2/macos
cmake --build build
```

The build produces an `ht-music.app` bundle inside `build`.
