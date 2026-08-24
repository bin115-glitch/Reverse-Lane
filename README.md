<div align="center">

![MCU](https://img.shields.io/badge/MCU-STM32L151CBT6-03234B?style=flat-square&logo=stmicroelectronics)
![Display](https://img.shields.io/badge/OLED-128x64_1--bit-white?style=flat-square&labelColor=black)
![Firmware](https://img.shields.io/badge/Firmware-Reverse_Lane-blue?style=flat-square)
![Language](https://img.shields.io/badge/Language-C%2FC%2B%2B-orange?style=flat-square)

# Reverse Lane

**1-bit OLED lane-dodge game for the AK Embedded Base Kit STM32L151**

Drive against traffic, dodge incoming vehicles, jump over obstacles and survive as the speed increases.

</div>

<p align="center">
  <img src="resources/images/readme_visual_overview_v3.svg" alt="Reverse Lane visual overview" width="850"/>
</p>

## Table of Contents

| No. | Section | Description |
|---:|---|---|
| 1 | [Hardware](#i-hardware) | Board images, MCU specification and flash partition layout |
| 2 | [Introduction](#introduction) | Game overview and project purpose |
| 3 | [Demo](#demo) | Demo video preview |
| 4 | [Main Features](#main-features) | Core firmware and game features |
| 5 | [Game Objects](#game-objects) | Player, jump player, moto, F1 and container bitmaps |
| 6 | [Basic Game Sequence Logic](#iv-basic-game-sequence-logic) | Time-ordered game messages, actions and render sequence |
| 7 | [Build and Flash](#build-and-flash) | Build command and STM32CubeProgrammer addresses |
| 8 | [Project Structure](#project-structure) | Main folders in the repository |
| 9 | [Documentation](#documentation) | Detailed Markdown guides and DOCX report |

## I. Hardware

<p align="center">
  <img width="343" height="319" alt="AK Embedded Base Kit STM32L151" src="https://github.com/user-attachments/assets/f015bf1e-d096-4f00-a36f-6b97a3643bb0"/>
</p>

<p align="center"><b>Figure 1:</b> AK Embedded Base Kit - STM32L151</p>

<p align="center">
  <img width="853" height="473" alt="AK Embedded Base Kit hardware layout" src="https://github.com/user-attachments/assets/7f76fc2a-659a-4de8-a99b-497f18e39d33"/>
</p>

The **AK Embedded Base Kit** is an evaluation kit for embedded software learners. It integrates a 1.54-inch OLED LCD, 3 push buttons and a buzzer, which are enough to study event-driven systems through a small real-time game. The board also exposes RS485, Qwiic and Grove connectors for broader prototyping.

### Specifications

| Item | Value |
|---|---|
| MCU | STM32L151CBT6 |
| CPU | Arm Cortex-M3 |
| RAM | 16 KB |
| Flash | 128 KB |
| Display | 128 x 64 monochrome OLED |
| Input | SW2, SW3, SW4 |

### Flash Partition Layout

| Memory Range | Size | Partition | Description |
|---|---:|---|---|
| `0x08000000 - 0x08001FFF` | 8 KB | Bootloader | AK bootloader partition |
| `0x08002000 - 0x08002FFF` | 4 KB | BSF Shared | Shared data between bootloader and application |
| `0x08003000 - 0x0801FFFF` | 116 KB | Application | Reverse Lane firmware |

## Introduction

**Reverse Lane** is a small embedded game built on the AK event-driven firmware architecture. The player controls a vehicle moving in the reverse direction while traffic approaches from the opposite side. The game uses real 1-bit bitmap sprites, lane-based collision logic, jump handling and score-based difficulty progression.

The project is designed for the **AK Embedded Base Kit STM32L151** with a **128 x 64 monochrome OLED**.

## Demo

<div align="center">
  <video
    src="https://github.com/user-attachments/assets/5d92aeeb-a82b-4d70-9fbb-fb683e4e7e60"
    controls
    width="700">
  </video>
</div>

## Main Features

- Event-driven screen flow using AK messages and timers.
- OLED frames rendered from 1-bit C/C++ bitmap arrays.
- Three-lane road layout with incoming traffic.
- Player movement, lane switching and jump animation.
- Moto, F1 and container objects with different sizes and behavior.
- Score-based level progression and increasing speed.
- Game-over, restart and menu navigation flow.

## Game Objects

| Preview | Object | Bitmap | Size | Behavior |
|:---:|---|---|---:|---|
| <img src="resources/images/bitmap/player.svg" width="85"/> | Player | `bitmap_player` | `17 x 18` | Controlled by SW2, SW3 and SW4 |
| <img src="resources/images/bitmap/player_jump.svg" width="120"/> | Jump Player | `bitmap_player_large` | `24 x 17` | Used during jump animation |
| <img src="resources/images/bitmap/moto.svg" width="90"/> | Moto | `bitmap_moto` | `18 x 10` | Small traffic object, can change lanes |
| <img src="resources/images/bitmap/f1.svg" width="135"/> | F1 | `bitmap_f1` | `27 x 10` | Fast traffic object, can change lanes |
| <img src="resources/images/bitmap/container.svg" width="200"/> | Container | `bitmap_container` | `40 x 13` | Long obstacle, harder to jump over |

Bitmap assets are converted from images into C/C++ byte arrays and drawn with `drawBitmap()`.

## IV. Basic Game Sequence Logic

The diagram below shows the **runtime flow** of Reverse Lane: screen entry, periodic game tick, player button actions, collision handling, game-over restart and exit back to the menu.

> **Note:** For a more detailed sequence flow, see [Runtime Signal Processing](docs/03-design-sequence-runtime.md).

```mermaid
%%{init: {'theme':'dark', 'sequence': {'actorMargin': 50, 'noteMargin': 10}}}%%
sequenceDiagram
    autonumber
    actor Player
    participant AK as AK Kernel
    participant Scr as scr_game_handle()
    participant Game as Game State
    participant Obj as Vehicle Pool
    participant View as view_scr_game()
    participant OLED as OLED

    rect rgb(30, 90, 60)
        Note left of Player: SCREEN_ENTRY
        AK->>Scr: SCREEN_ENTRY
        activate Scr
        Scr->>Game: game_init()
        Scr->>Obj: reset player + vehicles
        Scr->>AK: timer_set(AC_DISPLAY_GAME_TICK, 80 ms)
        deactivate Scr
    end

    rect rgb(85, 45, 115)
        Note left of Player: GAME PLAY
        AK->>Scr: AC_DISPLAY_GAME_TICK
        activate Scr
        Scr->>Game: game_update()
        Game->>Obj: move vehicles / spawn traffic
        Game->>Game: update level + score
        Game->>Game: check collision + jump state
        Game->>View: render current frame
        View->>OLED: drawBitmap() + update()
        deactivate Scr
    end

    rect rgb(30, 85, 135)
        Note left of Player: PLAYER ACTION
        Player->>AK: SW3 / UP
        AK->>Scr: AC_DISPLAY_BUTON_UP_PRESSED
        Scr->>Game: move player to upper lane

        Player->>AK: SW2 / DOWN
        AK->>Scr: AC_DISPLAY_BUTON_DOWN_PRESSED
        Scr->>Game: move player to lower lane

        Player->>AK: SW4 / MODE
        AK->>Scr: AC_DISPLAY_BUTON_MODE_PRESSED
        Scr->>Game: start jump / restart if game over
    end

    rect rgb(135, 45, 55)
        Note left of Player: GAME OVER / EXIT
        Game->>Game: game_over = true
        Player->>AK: SW4 / MODE
        AK->>Scr: AC_DISPLAY_BUTON_MODE_PRESSED
        Scr->>Game: game_init()

        Player->>AK: Long SW4 / MODE
        AK->>Scr: AC_DISPLAY_BUTON_MODE_LONG_PRESSED
        Scr->>AK: timer_remove_attr(AC_DISPLAY_GAME_TICK)
        Scr->>Scr: SCREEN_TRAN(scr_startup_handle)
    end
```

<p align="center"><strong><em>Figure:</em></strong> Basic game sequence logic</p>

Main game code:

```text
application/sources/app/screens/scr_game.cpp
```

## Build and Flash

Build the application firmware from Linux:

```bash
cd application
make clean
make
```

Flash with STM32CubeProgrammer:

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
|-- docs/             # Markdown guides and DOCX report
|-- README.md
```

## Documentation

| Document | Purpose |
|---|---|
| [Getting Started Guide](docs/01-guide-getting-started.md) | Build, flash, project structure and where to start reading code |
| [Game Object Sequences](docs/02-design-sequence-object.md) | Detailed object design for Player, Jump Player, Moto, F1, Container and vehicle pool |
| [Runtime Signal Processing](docs/03-design-sequence-runtime.md) | AK message flow, button events, 80 ms game tick, render flow and game-over sequence |
| [DOCX Project Report](docs/Reverse_Lane_Game_Document.docx) | Full Word document for presentation/report submission |

## References

| Topic | Link |
|---|---|
| AK Embedded Base Kit | https://epcb.vn/products/ak-embedded-base-kit-lap-trinh-nhung-vi-dieu-khien-mcu |
| AK Blog and Tutorial | https://epcb.vn/blogs/ak-embedded-software |
