<p align="center">
  <img width="560" height="144" alt="image" src="https://github.com/user-attachments/assets/7ac73801-ef7b-4cc1-8442-a191a2a0a1ce" />
</p>

---

[![Discord Channel](https://img.shields.io/discord/518540764754608128?color=%237289DA&logo=discord&logoColor=%23FFFFFF)](https://discord.gg/devilutionx-518540764754608128)
[![Downloads](https://img.shields.io/github/downloads/diasurgical/devilutionX/total.svg)](https://github.com/diasurgical/devilutionX/releases/latest)
[![Codecov](https://codecov.io/gh/diasurgical/devilutionX/branch/master/graph/badge.svg)](https://codecov.io/gh/diasurgical/devilutionX)

<p align="center">
<img width="853" height="480" alt="image" src="https://github.com/user-attachments/assets/ee902926-6382-4ee5-b1c2-7947e8b434e9" />
</p>

<sub>*(The health-bar and XP-bar are off by default but can be enabled in the [game settings](https://github.com/diasurgical/DevilutionX/wiki/Config-File). Widescreen can also be disabled if preferred.)*</sub>

# DevilutionX Free Movement

This is a fork of DevilutionX that makes walking feel closer to Diablo II and Diablo III. Your hero is no longer locked to the tile grid: they walk smoothly in any direction with both mouse and controller.

Turn it on in **Settings → Gameplay → Free Movement**. It is off by default. With it off, the game walks from tile to tile like regular DevilutionX.

## Free Movement

- **Walk in any direction.** The hero has an exact position inside each tile instead of jumping from tile to tile. They go straight toward where you point and turn right away, without first finishing a step.
- **Mouse.** Click or hold the button and the hero walks straight to the cursor. If something is in the way, they walk around it and cut corners wherever the way is clear. They slide along walls instead of stopping.
- **Controller.** The hero walks in the exact direction of the left stick, not just 8 directions.
  - Walking starts at a light push of the stick.
  - The hero turns with the stick in steps of about 2°.
  - Releasing the stick stops the hero on the spot.
- **Speed.**
  - Left, right, up and down on screen keep the original walking speed.
  - The four screen diagonals are 25% faster than the original, to even out the speeds a little.
  - Within 5° of any of these 8 directions, the hero walks at that direction's full speed. Directions in between blend smoothly.
- **Smooth drawing and animation.**
  - The hero and the camera move smoothly at any frame rate, not just at the game's 20 updates per second.
  - The walk animation plays as fast as the hero covers ground, so their feet don't slide.
  - The hero faces the way they actually walk, without flickering between two directions.
- **Original walking where it's needed.** Walking up to a monster, a townsperson, an item or a chest still goes tile by tile, starting from wherever the hero stands. With Free Movement on, those steps are drawn along a smooth line.

## Always on

These fixes apply whether or not Free Movement is on. They don't change walking speed or the number of steps to a destination:

- Holding the mouse button no longer makes the hero stop briefly between steps.
- Holding the mouse over a house, tree or other obstacle walks to the nearest reachable spot instead of standing still.
- Paths follow a straight line instead of an L shape.
- With a controller, walking queues several steps ahead and slides along walls.

## Known limitations

- Diablo's hero sprites only have 8 directions, so the hero always faces the closest of those 8.
- Free Movement adds a new network message, so multiplayer with regular DevilutionX builds is not supported. Multiplayer between players on this fork is untested.
- Recorded demos (such as the `Timedemo` test) made with regular DevilutionX no longer play back the same way.

---

# What is DevilutionX

DevilutionX is a port of Diablo and Hellfire that strives to make it simple to run the game while providing engine improvements, bug fixes, and some optional quality of life features.

Check out the [manual](https://github.com/diasurgical/devilutionX/wiki) for available features and how to take advantage of them.

For a full list of changes, see our [changelog](docs/CHANGELOG.md).

# How to Install

Note: You'll need access to the data from the original game. If you don't have an original CD, you can [buy Diablo from GoG.com](https://www.gog.com/game/diablo) or Battle.net. Alternatively, you can use `spawn.mpq` from the [shareware](https://github.com/diasurgical/devilutionx-assets/releases/latest/download/spawn.mpq) [[2]](http://ftp.blizzard.com/pub/demos/diablosw.exe) version, in place of `DIABDAT.MPQ`, to play the shareware portion of the game.

Download the latest [DevilutionX release](https://github.com/diasurgical/devilutionX/releases/latest) and extract the contents to a location of your choosing or [build from source](#building-from-source).

- Copy `DIABDAT.MPQ` from the CD or Diablo installation (or [extract it from the GoG installer](https://github.com/diasurgical/devilutionX/wiki/Extracting-MPQs-from-the-GoG-installer)) to the DevilutionX folder.
- To run the Diablo: Hellfire expansion, you will also need to copy `hellfire.mpq`, `hfmonk.mpq`, `hfmusic.mpq`, and `hfvoice.mpq`.

For more detailed instructions: [Installation Instructions](./docs/installing.md).

# Contributing

We are always looking for more people to help with [coding](docs/CONTRIBUTING.md), [documentation](https://github.com/diasurgical/devilutionX/wiki), [testing the latest builds](#test-builds), spreading the word, or simply just hanging out on our [Discord server](https://discord.gg/devilutionx-518540764754608128).

# Mods

We hope to provide a good starting point for mods. In addition to the full Devilution source code, we also provide modding tools. Check out the list of known [mods based on DevilutionX](https://github.com/diasurgical/DevilutionX/wiki/Community-Mods).

# Test Builds

If you want to help test the latest development version (make sure to back up your files, as these may contain bugs), you can fetch the test build artifact from one of the build servers:

*Note: You must be logged into GitHub to download the attachments!*

[![Linux x86_64](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_x86_64.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_x86_64.yml?query=branch%3Amaster)
[![Linux AArch64](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_aarch64.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_aarch64.yml?query=branch%3Amaster)
[![Linux x86](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_x86.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_x86.yml?query=branch%3Amaster)
[![Linux x86_64 SDL1](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_x86_64_SDL1.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Linux_x86_64_SDL1.yml?query=branch%3Amaster)
[![macOS x86_64](https://github.com/diasurgical/devilutionX/actions/workflows/macOS_x86_64.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/macOS_x86_64.yml?query=branch%3Amaster)
[![Windows MinGW x64](https://github.com/diasurgical/devilutionX/actions/workflows/Windows_MinGW_x64.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Windows_MinGW_x64.yml?query=branch%3Amaster)
[![Windows MinGW x86](https://github.com/diasurgical/devilutionX/actions/workflows/Windows_MinGW_x86.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Windows_MinGW_x86.yml?query=branch%3Amaster)
[![Windows MSVC x64](https://github.com/diasurgical/devilutionX/actions/workflows/Windows_MSVC_x64.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Windows_MSVC_x64.yml?query=branch%3Amaster)
[![Android](https://github.com/diasurgical/devilutionX/actions/workflows/Android.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/Android.yml?query=branch%3Amaster)
[![iOS](https://github.com/diasurgical/devilutionX/actions/workflows/iOS.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/iOS.yml?query=branch%3Amaster)
[![PS4](https://github.com/diasurgical/devilutionX/actions/workflows/PS4.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/PS4.yml?query=branch%3Amaster)
[![Original Xbox](https://github.com/diasurgical/devilutionX/actions/workflows/xbox_nxdk.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/xbox_nxdk.yml?query=branch%3Amaster)
[![Xbox One/Series](https://github.com/diasurgical/devilutionX/actions/workflows/xbox_one.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/xbox_one.yml?query=branch%3Amaster)
[![Nintendo Switch](https://github.com/diasurgical/devilutionX/actions/workflows/switch.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/switch.yml)
[![Sony PlayStation Vita](https://github.com/diasurgical/devilutionX/actions/workflows/vita.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/vita.yml)
[![Nintendo 3DS](https://github.com/diasurgical/devilutionX/actions/workflows/3ds.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/3ds.yml)
[![Amiga M68K](https://github.com/diasurgical/devilutionX/actions/workflows/amiga-m68k.yml/badge.svg)](https://github.com/diasurgical/devilutionX/actions/workflows/amiga-m68k.yml)

# Building from Source

Want to compile the program by yourself? Great! Simply follow the [build instructions](./docs/building.md).

# Credits

- The original Devilution project: [Devilution](https://github.com/diasurgical/devilution#credits)
- [Everyone](https://github.com/diasurgical/devilutionX/graphs/contributors) who worked on Devilution/DevilutionX
- [Nikolay Popov](https://www.instagram.com/nikolaypopovz/) for UI and graphics
- [WiAParker](https://wiaparker.pl/projekty/diablo-hellfire/) for the Polish voice pack
- And thanks to all who support the project, report bugs, and help spread the word ❤️

# Legal

DevilutionX is made publicly available and released under the Sustainable Use License (see [LICENSE](LICENSE.md)).

The source code in this repository is for non-commercial use only. If you use the source code, you may not charge others for access to it or any derivative work thereof.

Diablo® - Copyright © 1996 Blizzard Entertainment, Inc. All rights reserved. Diablo and Blizzard Entertainment are trademarks or registered trademarks of Blizzard Entertainment, Inc. in the U.S. and/or other countries.

DevilutionX and any of its maintainers are in no way associated with or endorsed by Blizzard Entertainment®.
