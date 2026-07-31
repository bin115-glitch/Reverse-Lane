<div align="center">

![MCU](https://img.shields.io/badge/MCU-STM32L151CBT6-03234B?style=flat-square&logo=stmicroelectronics)
![Display](https://img.shields.io/badge/OLED-128x64_1--bit-white?style=flat-square&labelColor=black)
![Firmware](https://img.shields.io/badge/Firmware-Fusion_Flight-blue?style=flat-square)
![Language](https://img.shields.io/badge/Language-C%2FC%2B%2B-orange?style=flat-square)

</div>

# Fusion Flight - Lane Dodge Game on AK Embedded Base Kit

<p align="center">
  <img src="hardware/images/ak-embedded-base-kit-version-3.jpg" alt="AK Embedded Base Kit" width="640"/>
</p>

---

## Documentation

| File | Description |
|---|---|
| [README.md](README.md) | Main project overview, hardware information, gameplay rules, bitmap assets, build and flashing guide. |
| [application/sources/app/screens/scr_startup.cpp](application/sources/app/screens/scr_startup.cpp) | Startup logo and menu screen. |
| [application/sources/app/screens/scr_game.cpp](application/sources/app/screens/scr_game.cpp) | Main gameplay logic, vehicle bitmaps, collision, jump and difficulty system. |
| [application/sources/common/view_render.cpp](application/sources/common/view_render.cpp) | OLED rendering wrapper above Adafruit GFX. |
| [application/sources/app/task_display.cpp](application/sources/app/task_display.cpp) | Display task and button signal dispatch flow. |

## Introduction

**Fusion Flight** is a 1-bit lane-dodge arcade game built on the **AK Embedded Base Kit STM32L151**. The player controls a small vehicle moving against traffic, dodges incoming vehicles across multiple lanes, jumps over obstacles and survives as the game speed increases.

The project is used to practice embedded game programming concepts:

- **Event-driven design:** Buttons are converted into AK messages and dispatched to active screens.
- **Timer-based gameplay:** The game updates from a periodic software timer instead of blocking delays.
- **OLED rendering:** All screens are drawn into a 1-bit framebuffer before being pushed to the OLED.
- **Fixed memory thinking:** Vehicle objects are stored in a fixed-size pool instead of dynamic allocation.
- **Bitmap assets:** Pixel art is converted into C/C++ byte arrays and drawn with `drawBitmap()`.
- **Collision tuning:** Each sprite has its own hitbox so gameplay feels fair on a small display.

## Hardware

Fusion Flight targets the AK Embedded Base Kit with an STM32L151CBT6 MCU and a monochrome OLED.

<table align="center">
  <tr>
    <td align="center"><img src="hardware/images/board-view-top-bottom.png" alt="AK Embedded Base Kit board view" width="900"/></td>
  </tr>
</table>
<p align="center"><strong><em>Figure 1:</em></strong> AK Embedded Base Kit - top and bottom board view.</p>

### MCU Overview

```text
SoC Name : STM32L151CBT6
CPU      : Arm Cortex-M3
Flash    : 128 KB
RAM      : 16 KB
Display  : 128 x 64 monochrome OLED
Input    : 3 push buttons
```

### Flash Memory Map

```text
[ 0x08000000 - 0x08001FFF ] : Bootloader partition
[ 0x08002000 - 0x08002FFF ] : BSF shared partition
[ 0x08003000 - 0x0801FFFF ] : Application partition
                                  Fusion Flight firmware
```

When flashing with STM32CubeProgrammer:

| Binary | Address |
|---|---:|
| `ak-base-kit-stm32l151-boot.bin` | `0x08000000` |
| `ak-base-kit-stm32l151-application.bin` | `0x08003000` |

## Game Description

The OLED framebuffer is **128 x 64 pixels, 1-bit**. Fusion Flight uses a road with **4 lanes**. The player vehicle stays near the left side and can move between lanes, jump forward, and dodge vehicles moving from right to left.

### Current Runtime Constants

| Constant | Value | Meaning |
|---|---:|---|
| `ROAD_TOP_Y` | `7` | First road pixel row. |
| `ROAD_LANE_COUNT` | `4` | Number of lanes. |
| `LANE_HEIGHT` | `14 px` | Height of each lane. |
| `MAX_VEHICLES` | `7` | Fixed vehicle pool size. |
| `GAME_TICK_INTERVAL_MS` | `80 ms` | Periodic game update timer. |
| `PLAYER_JUMP_TICKS` | `16` | Jump animation duration. |

## How to Play

| Button / Signal | Action |
|---|---|
| `UP` / `SW3` | Move player to the upper lane. |
| `DOWN` / `SW2` | Move player to the lower lane. |
| `MODE` / `SW4` | Jump. At Game Over, restart. |
| Long `MODE` | Return to startup menu. |

Gameplay rules:

- The player scores when a vehicle is successfully passed.
- Moto and F1 vehicles can change lane at higher levels.
- Container vehicles are longer and harder to jump over.
- Jumping over large vehicles requires enough speed or a higher level.
- At Game Over, press UP, DOWN or MODE to restart.

## Difficulty System

| Level | Score Required | Behavior |
|---|---:|---|
| Level 1 | `0` | Basic vehicle speed and spawn rate. |
| Level 2 | `10` | Faster traffic; small vehicles start changing lanes. |
| Level 3 | `50` | Higher speed; large vehicles can be jumped over with enough motion. |
| Level 4 | `100` | Highest speed; denser traffic and occasional double spawn. |

Difficulty is implemented in:

```text
application/sources/app/screens/scr_game.cpp
```

Main functions:

```cpp
static void game_update_difficulty();
static int level_speed_bonus();
static void vehicle_try_change_lane(int index);
static bool jump_has_clear_speed();
```

## Game Objects

| Object | Bitmap | Size | Speed Step | Hitbox | Jumpable | Lane Change |
|---|---|---:|---:|---:|---|---|
| Player | `bitmap_player` | `17 x 18 px` | controlled | `x + 4`, width `11` | yes | manual |
| Jump Player | `bitmap_player_large` | `24 x 17 px` | controlled | animation only | yes | manual |
| Moto | `bitmap_moto` | `18 x 10 px` | `2` | `x + 3`, width `12` | yes | yes |
| F1 | `bitmap_f1` | `27 x 10 px` | `3` | `x + 4`, width `19` | yes | yes |
| Container | `bitmap_container` | `40 x 13 px` | `1` | `x + 4`, width `32` | level gated | no |

Vehicle type definition:

```cpp
struct game_vehicle_type_t {
    const unsigned char* bitmap;
    int width;
    int height;
    int step;
    int hit_x;
    int hit_w;
    bool jumpable;
    bool can_change_lane;
};
```

Configured vehicle types:

```cpp
static const game_vehicle_type_t vehicle_types[] = {
    {bitmap_moto,      18, 10, 2, 3, 12, true,  true},
    {bitmap_f1,        27, 10, 3, 4, 19, true,  true},
    {bitmap_container, 40, 13, 1, 4, 32, false, false},
};
```

## Bitmap Assets

The OLED is monochrome, so PNG/JPG assets are not loaded directly. Each asset must be converted into a C/C++ byte array and compiled into firmware.

Recommended conversion settings:

```text
Tool                : Image2CPP
Code output format  : Arduino code
Draw mode           : Horizontal - 1 bit per pixel
Canvas              : Match the intended object size
Background          : Black or Transparent
```

Recommended asset sizes:

| Object Type | Suggested Size |
|---|---:|
| Player | `17 x 18 px` |
| Small vehicle | `18 x 10 px` |
| F1 / fast vehicle | `27 x 10 px` |
| Container | `40 x 13 px` |

Example bitmap:

```cpp
static const unsigned char PROGMEM bitmap_f1[] = {
    0x03, 0x80, 0x07, 0x00,
    0x1f, 0xcf, 0xff, 0x00,
    // ... remaining bytes ...
};
```

Example draw call:

```cpp
int y = lane_center_y(vehicles[i].lane) - vehicles[i].type->height / 2;
view_render.drawBitmap(
    vehicles[i].x,
    y,
    vehicles[i].type->bitmap,
    vehicles[i].type->width,
    vehicles[i].type->height,
    WHITE
);
```

Important notes:

- `width` and `height` must match the converted bitmap.
- A wrong size can make OLED output look broken or read bytes from the next bitmap.
- One lane is currently `14 px` high. Any object taller than `14 px` may overlap lane lines.
- Build outputs such as `.bin`, `.elf`, `.axf`, `.o` and `.map` should not be committed to Git.

## Build on Linux

Build the application firmware:

```bash
cd ~/Documents/snake/application
make clean
make
```

The application binary is generated at:

```text
application/build_ak-base-kit-stm32l151-application/ak-base-kit-stm32l151-application.bin
```

Build the bootloader only when boot code changes:

```bash
cd ~/Documents/snake/boot
make clean
make
```

The boot binary is generated at:

```text
boot/build_ak-base-kit-stm32l151-boot/ak-base-kit-stm32l151-boot.bin
```

If `objcopy` reports `No space left on device`, check Linux disk usage:

```bash
df -h
sudo apt clean
sudo apt autoremove
```

## Flashing

Using STM32CubeProgrammer:

1. Connect with ST-LINK over SWD.
2. Flash bootloader at `0x08000000` if required.
3. Flash application at `0x08003000`.
4. Enable verify.
5. Use hardware reset or power cycle after flashing.

Typical application-only flashing:

```text
File          : ak-base-kit-stm32l151-application.bin
Start address : 0x08003000
```

## Project Structure

```text
snake/
|-- application/
|   |-- sources/app/screens/
|   |   |-- scr_startup.cpp
|   |   |-- scr_game.cpp
|   |-- sources/common/view_render.cpp
|   |-- sources/app/task_display.cpp
|-- boot/
|-- hardware/
|   |-- bin/
|   |-- images/
|   |-- schematic/
|-- README.md
```

## Git Setup

```bash
git init
git add .
git commit -m "Initial Fusion Flight firmware"
```

The repository includes a `.gitignore` that excludes build outputs and temporary files.

## References

| Topic | Link |
|---|---|
| AK Embedded Base Kit | https://epcb.vn/products/ak-embedded-base-kit-lap-trinh-nhung-vi-dieu-khien-mcu |
| AK Blog and Tutorial | https://epcb.vn/blogs/ak-embedded-software |
| Bug_Storm reference style | https://github.com/letuandat269/Bug_Storm |