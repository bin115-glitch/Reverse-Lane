<div align="center">

![MCU](https://img.shields.io/badge/MCU-STM32L151CBT6-03234B?style=flat-square&logo=stmicroelectronics)
![Display](https://img.shields.io/badge/OLED-128x64_1--bit-white?style=flat-square&labelColor=black)
![Firmware](https://img.shields.io/badge/Firmware-Fusion_Flight-blue?style=flat-square)
![Language](https://img.shields.io/badge/Language-C%2FC%2B%2B-orange?style=flat-square)

</div>

# Fusion Flight

**Fusion Flight** is a small 1-bit lane-dodge game for the **AK Embedded Base Kit STM32L151**. The player controls a vehicle moving against traffic, dodges incoming vehicles, jumps over obstacles and survives as the game becomes faster.

<p align="center">
  <img src="resources/images/readme_visual_overview.svg" alt="Fusion Flight visual overview" width="850"/>
</p>

<p align="center">
  <img src="hardware/images/ak-embedded-base-kit-version-3.jpg" alt="AK Embedded Base Kit" width="620"/>
</p>

## Main Features

- Event-driven screen flow using AK messages.
- Timer-based gameplay with `AC_DISPLAY_GAME_TICK` every `80 ms`.
- 128 x 64 monochrome OLED rendering through a 1-bit framebuffer.
- 4-lane traffic dodge gameplay.
- Player jump, restart and menu return controls.
- Fixed vehicle pool with Moto, F1 and Container objects.
- Difficulty levels based on score: `0`, `10`, `50`, `100`.

## Hardware Target

```text
MCU      : STM32L151CBT6
CPU      : Arm Cortex-M3
Flash    : 128 KB
RAM      : 16 KB
Display  : 128 x 64 monochrome OLED
Input    : SW2, SW3, SW4
```

Flash layout:

| Binary | Address |
|---|---:|
| `ak-base-kit-stm32l151-boot.bin` | `0x08000000` |
| `ak-base-kit-stm32l151-application.bin` | `0x08003000` |

## Gameplay

<p align="center">
  <img src="resources/images/screens/scr_gameplay_layout.svg" alt="Fusion Flight OLED gameplay layout" width="760"/>
</p>

Controls:

| Button | Action |
|---|---|
| `SW3 / UP` | Move up one lane |
| `SW2 / DOWN` | Move down one lane |
| `SW4 / MODE` | Jump / restart after Game Over |
| Long `SW4 / MODE` | Back to menu |

Rules:

- Score increases when the player successfully passes a vehicle.
- Moto and F1 can change lanes at higher levels.
- Container is longer and harder to clear.
- Jumping can avoid collision if the object is jumpable or the player has enough speed.

## Video Demo

<div align="center">
  <video
    src="https://github.com/user-attachments/assets/5d92aeeb-a82b-4d70-9fbb-fb683e4e7e60"
    controls
    width="700">
  </video>
</div>

## Game Objects

| Preview | Object | Bitmap | Size | Notes |
|:---:|---|---|---:|---|
| <img src="resources/images/bitmap/player.svg" width="85"/> | Player | `bitmap_player` | `17 x 18` | Controlled by buttons |
| <img src="resources/images/bitmap/player_jump.svg" width="120"/> | Jump Player | `bitmap_player_large` | `24 x 17` | Jump animation |
| <img src="resources/images/bitmap/moto.svg" width="90"/> | Moto | `bitmap_moto` | `18 x 10` | Small vehicle, lane change |
| <img src="resources/images/bitmap/f1.svg" width="135"/> | F1 | `bitmap_f1` | `27 x 10` | Fast vehicle, lane change |
| <img src="resources/images/bitmap/container.svg" width="200"/> | Container | `bitmap_container` | `40 x 13` | Large vehicle |

Bitmap assets are converted from images into C/C++ byte arrays and drawn with `drawBitmap()`.

## Runtime Flow

```mermaid
flowchart LR
    Input["Buttons"]
    Kernel["AK Kernel<br/>Message + Timer"]
    Screen["scr_game_handle()"]
    Init["game_init()"]
    Update["game_update()"]
    Vehicles["Vehicle Pool[7]"]
    Collision["Collision + Jump Check"]
    Render["view_scr_game()"]
    OLED["OLED 128 x 64"]

    Input --> Kernel --> Screen
    Screen -->|SCREEN_ENTRY| Init
    Screen -->|80 ms tick| Update
    Screen -->|button event| Update
    Init --> Vehicles
    Update --> Vehicles
    Vehicles --> Collision
    Collision --> Render --> OLED
```

Main gameplay code:

```text
application/sources/app/screens/scr_game.cpp
```

## Build

Build bootloader:

```bash
cd boot
make clean
make
```

Build application:

```bash
cd application
make clean
make
```

Generated binaries:

```text
boot/build_ak-base-kit-stm32l151-boot/ak-base-kit-stm32l151-boot.bin
application/build_ak-base-kit-stm32l151-application/ak-base-kit-stm32l151-application.bin
```

## Flash

Use STM32CubeProgrammer:

1. Flash bootloader at `0x08000000`.
2. Flash application at `0x08003000`.
3. Enable verify.
4. Reset or power-cycle the board.

For application-only updates, flash only:

```text
ak-base-kit-stm32l151-application.bin -> 0x08003000
```

## Project Structure

```text
snake/
|-- application/      # Application firmware and game screens
|-- boot/             # Bootloader firmware
|-- hardware/         # Board images, binaries and schematic assets
|-- resources/        # README bitmap previews and OLED layout preview
|-- README.md
```

## References

| Topic | Link |
|---|---|
| AK Embedded Base Kit | https://epcb.vn/products/ak-embedded-base-kit-lap-trinh-nhung-vi-dieu-khien-mcu |
| AK Blog and Tutorial | https://epcb.vn/blogs/ak-embedded-software |
