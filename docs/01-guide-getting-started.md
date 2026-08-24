<h1 align="center">Reverse Lane - Getting Started Guide</h1>

This document explains how to understand, build and flash **Reverse Lane** on the AK Embedded Base Kit STM32L151. It is written for reading the project code and presenting how the game is structured.

---

## Table of Contents

- [I. Project Overview](#i-project-overview)
- [II. Hardware Target](#ii-hardware-target)
- [III. Project Structure](#iii-project-structure)
- [IV. Build Environment](#iv-build-environment)
- [V. Build and Flash Workflow](#v-build-and-flash-workflow)
- [VI. Where to Start Reading Code](#vi-where-to-start-reading-code)
- [VII. Development Workflow](#vii-development-workflow)
- [VIII. Bitmap Workflow](#viii-bitmap-workflow)

---

## I. Project Overview

**Reverse Lane** is a 1-bit OLED lane-dodge game.

The player controls a vehicle moving against traffic. Incoming vehicles move from right to left across four lanes. The player can move between lanes and jump to avoid collision. Score increases when a vehicle is successfully passed. The game becomes faster at score milestones.

Main gameplay characteristics:

- Display: `128 x 64` monochrome OLED.
- Main loop: timer-driven by `AC_DISPLAY_GAME_TICK`.
- Tick interval: `80 ms`.
- Game style: event-driven using AK messages, timers and screen manager.
- Main gameplay file: `application/sources/app/screens/scr_game.cpp`.

---

## II. Hardware Target

| Item | Value |
|---|---|
| MCU | `STM32L151CBT6` |
| CPU | Arm Cortex-M3 |
| Display | 128 x 64 1-bit OLED |
| Input | SW2, SW3, SW4 |
| Boot address | `0x08000000` |
| Application address | `0x08003000` |

Button mapping used by the game:

| Button | Signal | Gameplay action |
|---|---|---|
| `SW3 / UP` | `AC_DISPLAY_BUTON_UP_PRESSED` | Move player up one lane / restart after game over |
| `SW2 / DOWN` | `AC_DISPLAY_BUTON_DOWN_PRESSED` | Move player down one lane / restart after game over |
| `SW4 / MODE` short | `AC_DISPLAY_BUTON_MODE_PRESSED` | Jump / restart / select menu item |
| `SW4 / MODE` long | `AC_DISPLAY_BUTON_MODE_LONG_PRESSED` | Return from game to menu |

---

## III. Project Structure

```text
snake/
|-- application/
|   |-- sources/
|   |   |-- app/
|   |   |   |-- app.cpp
|   |   |   |-- app.h
|   |   |   |-- app_bsp.cpp
|   |   |   |-- screens/
|   |   |   |   |-- scr_startup.cpp
|   |   |   |   |-- scr_game.cpp
|   |   |-- common/
|   |   |   |-- screen_manager.cpp
|   |   |-- view/
|   |   |   |-- view_render.cpp
|   |-- Makefile
|-- boot/
|-- docs/
|-- hardware/
|-- resources/
|-- README.md
```

Important folders:

| Folder | Meaning |
|---|---|
| `application/` | Main firmware application and game code |
| `boot/` | Bootloader project |
| `docs/` | Project documentation |
| `resources/` | README images and visual assets |
| `hardware/` | Board resources and binary output folder |

---

## IV. Build Environment

Build the firmware on Linux/Ubuntu or WSL/Linux with ARM GCC installed.

Check compiler:

```bash
which arm-none-eabi-gcc
arm-none-eabi-gcc --version
```

The project Makefile may reference a specific GCC path. If your machine uses a different ARM GCC version, update the Makefile path carefully.

Example issue:

```text
No rule to make target /usr/lib/gcc/arm-none-eabi/10.3.1/...
```

If your installed version is `14.2.1`, adjust the Makefile path from `10.3.1` to `14.2.1`.

---

## V. Build and Flash Workflow

### 1. Build application

```bash
cd ~/Documents/snake/application
make clean
make
```

Expected binary:

```text
application/build_ak-base-kit-stm32l151-application/ak-base-kit-stm32l151-application.bin
```

### 2. Build bootloader

Only rebuild bootloader when startup/bootloader code changes.

```bash
cd ~/Documents/snake/boot
make clean
make
```

Expected binary:

```text
boot/build_ak-base-kit-stm32l151-boot/ak-base-kit-stm32l151-boot.bin
```

### 3. Flash with STM32CubeProgrammer

Recommended flashing addresses:

| Binary | Address |
|---|---:|
| `ak-base-kit-stm32l151-boot.bin` | `0x08000000` |
| `ak-base-kit-stm32l151-application.bin` | `0x08003000` |

Normal gameplay update:

```text
Flash only application binary at 0x08003000.
```

Only full restore:

```text
Flash boot at 0x08000000, then application at 0x08003000.
```

Avoid full chip erase unless you are ready to restore both bootloader and application.

---

## VI. Where to Start Reading Code

Start with this order:

| Step | File | Why |
|---|---|---|
| 1 | `application/sources/app/app.cpp` | Initializes buttons, timers and screen manager |
| 2 | `application/sources/app/app_bsp.cpp` | Converts SW2/SW3/SW4 into AK display messages |
| 3 | `application/sources/common/screen_manager.cpp` | Dispatches messages to current screen and controls render timing |
| 4 | `application/sources/app/screens/scr_startup.cpp` | Startup logo and menu logic |
| 5 | `application/sources/app/screens/scr_game.cpp` | Main game logic, object bitmaps, movement, collision and rendering |
| 6 | `application/sources/view/view_render.cpp` | Render bridge to OLED/GFX layer |

---

## VII. Development Workflow

Suggested workflow when modifying the game:

1. Decide the gameplay change.
2. Check which state variables are affected in `scr_game.cpp`.
3. Modify bitmap/object data if needed.
4. Update movement/collision/spawn logic.
5. Build application on Linux.
6. Flash application binary to `0x08003000`.
7. Test on OLED.
8. Update docs if object rules or runtime flow changed.

Examples:

| Change | Best place to edit |
|---|---|
| Change player bitmap | `bitmap_player[]` in `scr_game.cpp` |
| Change enemy speed | `vehicle_types[]` and `level_speed_bonus()` |
| Change score milestones | `game_update_difficulty()` |
| Change collision fairness | `hit_x`, `hit_w`, and `game_check_collision()` |
| Change menu text | `scr_startup.cpp` |
| Change button behavior | `scr_game_handle()` or `app_bsp.cpp` |

---

## VIII. Bitmap Workflow

Bitmap assets are not loaded as PNG/JPG at runtime. They are converted into C/C++ byte arrays.

Recommended conversion tool:

```text
https://javl.github.io/image2cpp/
```

Recommended settings:

```text
Output format : Arduino code
Draw mode     : Horizontal - 1 bit per pixel
Color         : monochrome / binary
```

In firmware, the byte array is drawn with:

```cpp
view_render.drawBitmap(x, y, bitmap_player, PLAYER_WIDTH, PLAYER_HEIGHT, WHITE);
```

Current gameplay bitmaps are declared directly in:

```text
application/sources/app/screens/scr_game.cpp
```

Current object list:

| Object | Bitmap symbol | Size |
|---|---|---:|
| Player | `bitmap_player` | `17 x 18` |
| Jump Player | `bitmap_player_large` | `24 x 17` |
| Moto | `bitmap_moto` | `18 x 10` |
| F1 | `bitmap_f1` | `27 x 10` |
| Container | `bitmap_container` | `40 x 13` |

---

## References

- AK Embedded Base Kit STM32L151: `https://github.com/the-ak-foundation/ak-base-kit-stm32l151`
- Image to C bitmap converter: `https://javl.github.io/image2cpp/`
- Pixel art editor: `https://www.pixilart.com/`

