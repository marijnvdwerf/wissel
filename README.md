# Wissel
An open source re-implementation of Chris Sawyer's Locomotion. A construction and management simulation video game that simulates running a transport company.

---

# Contents
- 1 - [Introduction](#1-introduction)
- 2 - [Downloading the game (pre-built)](#2-downloading-the-game-pre-built)
- 3 - [Building the game](#3-building-the-game)
  - 3.1 - [Building prerequisites](#31-building-prerequisites)
  - 3.2 - [Compiling and running](#32-compiling-and-running)
- 4 - [Licence](#5-licence)
- 5 - [More information](#6-more-information)

---

### Build Status
|             | ~~Windows~~ | Linux / Mac | Download |
|-------------|---------|-------------|----------|
| **master**  |  | [![Travis CI](https://travis-ci.org/marijnvdwerf/wissel.svg?branch=master)](https://travis-ci.org/marijnvdwerf/wissel) | [![GitHub release](https://img.shields.io/github/release/marijnvdwerf/wissel.svg)](https://github.com/marijnvdwerf/wissel/releases) |

---

# 1 Introduction

**Wissel** is an open-source re-implementation of Chris Sawyer's Locomotion (CSL). CSL is the spiritual successor to Transport Tycoon and Wissel aims to improve the game similar to how [OpenTTD](http://openttd.org) improved Transport Tycoon and [OpenRCT2](http://openrct2.website) improved RollerCoaster Tycoon.

---

# 2 Downloading the game (pre-built)

Wissel requires original files of Chris Sawyer's Locomotion to play. It can be bought at either [Steam](http://store.steampowered.com/app/356430/) or [GOG.com](https://www.gog.com/game/chris_sawyers_locomotion).

The latest release can be found on [GitHub](https://github.com/marijnvdwerf/wissel/releases).

---

# 3 Building the game

## 3.1 Building prerequisites

Wissel requires original files of Chris Sawyer's Locomotion to play. It can be bought at either [Steam](http://store.steampowered.com/app/356430/) or [GOG.com](https://www.gog.com/game/chris_sawyers_locomotion).

### Windows:
- 7 / 8 / 10
- [Visual Studio 2017](https://www.visualstudio.com/vs/community/)
  - Desktop development with C++
  - Windows 10 SDK (10.0.14393.0)
- [SDL2](https://www.libsdl.org/download-2.0.php)
- [SDL2_mixer](https://www.libsdl.org/projects/SDL_mixer/)
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)

### Linux / macOS:
- cmake
- make or ninja
- 32-bit versions of the following:
  - [SDL2](https://www.libsdl.org/download-2.0.php)
  - [SDL2_mixer](https://www.libsdl.org/projects/SDL_mixer/)
  - [yaml-cpp](https://github.com/jbeder/yaml-cpp)
  - [Boost](http://www.boost.org/) (macOS only)

---

## 3.2 Compiling and running
### Windows:
1. Check out the repository. This can be done using [GitHub Desktop](https://desktop.github.com) or [other tools](https://help.github.com/articles/which-remote-url-should-i-use).
2. Install dependencies using [vcpkg](https://github.com/microsoft/vcpkg) or use the [nuget package](https://github.com/OpenRCT2/OpenLoco-Dependencies/releases).
3. Open a new Developer Command Prompt for VS 2017, then navigate to the repository (e.g. `cd C:\GitHub\OpenRCT2`).
4. Run `msbuild openloco.sln`
5. Run `mklink /D bin\data ..\data` or `xcopy data bin\data /EIY`
6. Run the game, `bin\openloco`

### Linux / macOS:
The standard CMake build procedure is to install the required libraries, then:
```
mkdir build
cd build
CXXFLAGS="-m32" cmake ..
make
```

Running the game will need the data directory from the root of the source code next to the binary. Assuming you're in `$SRC/build`, 
```
ln -s ../data
OR
cp -r ../data ./data 
```
---

# 4 Licence
**Wissel** is licensed under the MIT License.

---

# 5 More information
- [GitHub](https://github.com/marijnvdwerf/wissel)
- [TT-Forums](https://www.tt-forums.net)
- [Locomotion subreddit](https://www.reddit.com/r/locomotion/)
