<h1 align="center">Reverse Lane - Runtime Signal Processing</h1>

This document explains how **Reverse Lane** processes startup, button input, game ticks, object updates, rendering and game-over behavior. The game uses the AK event-driven task architecture, but the gameplay objects are managed inside the game screen instead of separate object tasks.

---

## Table of Contents

- [I. Runtime Overview](#i-runtime-overview)
- [II. Main Runtime Architecture](#ii-main-runtime-architecture)
- [III. Game Start Sequence](#iii-game-start-sequence)
- [IV. Game Playing Sequence](#iv-game-playing-sequence)
- [V. Button Event Processing](#v-button-event-processing)
- [VI. Render Processing](#vi-render-processing)
- [VII. Game Over and Restart Sequence](#vii-game-over-and-restart-sequence)
- [VIII. Per-Tick Update Order](#viii-per-tick-update-order)
- [IX. Task Ownership](#ix-task-ownership)
- [X. Code References](#x-code-references)

---

## I. Runtime Overview

Reverse Lane uses the AK framework as an event-driven system:

1. Hardware buttons or timers create software signals.
2. Signals are posted to an AK task, mainly `AC_TASK_DISPLAY_ID`.
3. AK scheduler dispatches the message to `task_display()`.
4. `task_display()` forwards the message to `screen_manager`.
5. `screen_manager` calls the current screen handler.
6. The current screen updates its state.
7. `screen_manager` requests a render if the render interval allows.
8. `view_render` draws to OLED.

Main runtime signal:

```cpp
AC_DISPLAY_GAME_TICK
```

Game tick interval:

```cpp
GAME_TICK_INTERVAL_MS = 80 ms
```

Render throttle:

```cpp
AC_DISPLAY_MINIMUM_SCREEN_RENDER_INTERVAL_MS = 50 ms
```

---

## II. Main Runtime Architecture

```mermaid
flowchart LR
    Button["SW2 / SW3 / SW4"]
    BSP["app_bsp.cpp<br/>button callback"]
    Timer["timer.c<br/>periodic / one-shot"]
    Queue["AK Message Queue"]
    Display["task_display()"]
    Manager["screen_manager<br/>scr_mng_dispatch()"]
    Screen["Current Screen<br/>scr_startup / scr_game"]
    Render["view_render_screen()"]
    OLED["OLED 128 x 64"]

    Button --> BSP --> Queue
    Timer --> Queue
    Queue --> Display --> Manager --> Screen
    Screen --> Manager --> Render --> OLED
```

Important idea:

```text
The BSP does not decide what screen is active.
It only posts button messages.
The active screen decides what each signal means.
```

---

## III. Game Start Sequence

Game start begins from the startup screen, not directly from `scr_game.cpp`.

Startup sequence:

```mermaid
sequenceDiagram
    participant App as app.cpp
    participant Timer
    participant Display as AC_TASK_DISPLAY_ID
    participant Startup as scr_startup_handle
    participant OLED as view_render/OLED
    participant Game as scr_game_handle

    App->>Timer: timer_set(AC_DISPLAY_INITIAL, 100ms, ONE_SHOT)
    Timer->>Display: AC_DISPLAY_INITIAL
    Display->>Startup: dispatch
    Startup->>OLED: initialize + display on
    Startup->>Startup: startup_show_logo = true
    Startup->>Timer: timer_set(AC_DISPLAY_SHOW_LOGO, 2000ms, ONE_SHOT)
    Startup->>OLED: draw logo bitmap
    Timer->>Display: AC_DISPLAY_SHOW_LOGO
    Display->>Startup: dispatch
    Startup->>Startup: startup_show_logo = false
    Startup->>OLED: draw menu
    Display->>Startup: AC_DISPLAY_BUTON_MODE_PRESSED
    Startup->>Game: SCREEN_TRAN(scr_game_handle, &scr_game)
    Game->>Game: SCREEN_ENTRY
```

Startup menu items:

```text
Reverse Lane
Cai dat
Thong ke
```

When `Reverse Lane` is selected, `SCREEN_TRAN()` changes current screen and sends `SCREEN_ENTRY` to the game screen.

---

## IV. Game Playing Sequence

On `SCREEN_ENTRY`, game screen initializes gameplay state and starts the 80 ms timer.

```cpp
case SCREEN_ENTRY: {
    game_init();
    timer_set(AC_TASK_DISPLAY_ID,
              AC_DISPLAY_GAME_TICK,
              GAME_TICK_INTERVAL_MS,
              TIMER_PERIODIC);
} break;
```

Gameplay sequence:

```mermaid
sequenceDiagram
    participant Timer
    participant AKOS as AK Scheduler
    participant Display as task_display
    participant ScreenManager as screen_manager
    participant Game as scr_game_handle
    participant Update as game_update
    participant Render as view_scr_game
    participant OLED

    note over Timer,Game: Periodic 80 ms gameplay tick
    Timer->>AKOS: AC_DISPLAY_GAME_TICK to AC_TASK_DISPLAY_ID
    AKOS->>Display: dispatch message
    Display->>ScreenManager: scr_mng_dispatch(msg)
    ScreenManager->>Game: current_screen(msg)
    alt game_over == false
        Game->>Update: game_update()
        Update->>Update: difficulty / player / spawn / vehicle / score / collision
    else game_over == true
        Game->>Game: ignore game tick
    end
    ScreenManager->>Render: view_render_screen(scr_game)
    Render->>OLED: draw HUD, lanes, vehicles, player, overlay
```

The game keeps objects inside the screen file:

```text
scr_game.cpp
|-- player state
|-- bitmap arrays
|-- vehicle_types[]
|-- vehicles[MAX_VEHICLES]
|-- game_update()
|-- game_check_collision()
|-- view_scr_game()
```

---

## V. Button Event Processing

Button callbacks post signals to the display task.

| Button | BSP callback | Posted signal | Destination |
|---|---|---|---|
| SW4 / MODE short | `btn_mode_callback()` | `AC_DISPLAY_BUTON_MODE_PRESSED` | `AC_TASK_DISPLAY_ID` |
| SW4 / MODE long | `btn_mode_callback()` | `AC_DISPLAY_BUTON_MODE_LONG_PRESSED` | `AC_TASK_DISPLAY_ID` |
| SW3 / UP | `btn_up_callback()` | `AC_DISPLAY_BUTON_UP_PRESSED` | `AC_TASK_DISPLAY_ID` |
| SW2 / DOWN | `btn_down_callback()` | `AC_DISPLAY_BUTON_DOWN_PRESSED` | `AC_TASK_DISPLAY_ID` |

Button sequence:

```mermaid
sequenceDiagram
    participant Button
    participant BSP as app_bsp.cpp
    participant AKOS
    participant Display as task_display
    participant Manager as screen_manager
    participant Screen as active screen

    Button->>BSP: physical press
    BSP->>AKOS: task_post_pure_msg(AC_TASK_DISPLAY_ID, AC_DISPLAY_BUTON_*_PRESSED)
    AKOS->>Display: dispatch
    Display->>Manager: scr_mng_dispatch(msg)
    Manager->>Screen: screen(msg)
    alt active screen is startup
        Screen->>Screen: menu navigation or transition
    else active screen is game
        Screen->>Screen: lane move / jump / restart / back
    end
```

Game screen button behavior:

| Signal | Game state | Action |
|---|---|---|
| `AC_DISPLAY_BUTON_UP_PRESSED` | `game_over == true` | Restart game |
| `AC_DISPLAY_BUTON_UP_PRESSED` | grounded | Move up one lane |
| `AC_DISPLAY_BUTON_DOWN_PRESSED` | `game_over == true` | Restart game |
| `AC_DISPLAY_BUTON_DOWN_PRESSED` | grounded | Move down one lane |
| `AC_DISPLAY_BUTON_MODE_PRESSED` | `game_over == true` | Restart game |
| `AC_DISPLAY_BUTON_MODE_PRESSED` | grounded | Start jump |
| `AC_DISPLAY_BUTON_MODE_LONG_PRESSED` | in game | Stop game tick and return to menu |

---

## VI. Render Processing

Rendering is controlled by `screen_manager.cpp`.

After every dispatched display message:

```cpp
screen_manager->screen(msg);
scr_mng_render_screen();
```

Render sequence:

```mermaid
sequenceDiagram
    participant Display as task_display
    participant Manager as screen_manager
    participant Screen as current screen handler
    participant View as view_screen_t
    participant Render as view_render_screen
    participant OLED
    participant Timer

    Display->>Manager: scr_mng_dispatch(msg)
    Manager->>Screen: screen(msg)
    Manager->>Manager: check current_ms - screen_last_render_ms
    alt first render or >= 50 ms
        Manager->>Render: view_render_screen(view_screen)
        Render->>View: call dynamic render callback
        View->>OLED: draw framebuffer
    else too soon
        Manager->>Timer: timer_set(AC_DISPLAY_RENDER_SCREEN, remaining_ms, ONE_SHOT)
    end
```

Game render draws:

1. Score and level HUD.
2. Four lane bands using horizontal lines.
3. Active vehicles from `vehicles[]`.
4. Player or jump-player bitmap.
5. Game Over overlay if needed.

---

## VII. Game Over and Restart Sequence

Game over happens inside `game_update()` after collision check.

```cpp
if (game_check_collision()) {
    game_over = true;
    timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_TICK);
}
```

Game-over sequence:

```mermaid
sequenceDiagram
    participant Update as game_update
    participant Collision as game_check_collision
    participant Timer
    participant Render as view_scr_game
    participant Button
    participant Game as scr_game_handle

    Update->>Collision: check active vehicles
    alt collision cannot be cleared
        Collision->>Update: true
        Update->>Game: game_over = true
        Update->>Timer: timer_remove_attr(AC_DISPLAY_GAME_TICK)
        Render->>Render: draw GAME OVER overlay
    end

    Button->>Game: UP / DOWN / MODE pressed
    alt game_over == true
        Game->>Game: game_init()
        Game->>Timer: timer_set(AC_DISPLAY_GAME_TICK, 80ms, PERIODIC)
    end
```

Return to menu:

```cpp
case AC_DISPLAY_BUTON_MODE_LONG_PRESSED: {
    timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_TICK);
    SCREEN_TRAN(scr_startup_handle, &scr_startup);
} break;
```

---

## VIII. Per-Tick Update Order

Every `AC_DISPLAY_GAME_TICK` runs this order inside `game_update()`:

1. `game_update_difficulty()`
2. Check SW2 + SW3 hold return condition
3. Apply jump forward movement
4. Count down jump / returning / SW4 tap state
5. Decrement `spawn_counter`
6. If needed, call `game_spawn_vehicle()`
7. Level 4 may spawn one extra vehicle
8. For each active vehicle:
   - `vehicle_try_change_lane(i)`
   - Move x left by `vehicle.type->step + level_speed_bonus()`
   - Add score if vehicle fully passed player
   - Deactivate if off-screen
9. Run `game_check_collision()`
10. If collision is true:
    - `game_over = true`
    - remove game tick timer

Per-tick flow:

```mermaid
flowchart TD
    A["AC_DISPLAY_GAME_TICK"] --> B["game_update_difficulty()"]
    B --> C["Update player jump / return state"]
    C --> D["spawn_counter--"]
    D --> E{"spawn_counter <= 0?"}
    E -->|Yes| F["game_spawn_vehicle()"]
    E -->|No| G["Update active vehicles"]
    F --> G
    G --> H["vehicle_try_change_lane()"]
    H --> I["Move vehicles left"]
    I --> J["Score passed vehicles"]
    J --> K["Deactivate off-screen vehicles"]
    K --> L["game_check_collision()"]
    L --> M{"collision?"}
    M -->|No| N["Render next frame"]
    M -->|Yes| O["game_over = true<br/>remove game tick timer"]
```

---

## IX. Task Ownership

| Task / Module | Responsibility | Owns data | Receives |
|---|---|---|---|
| `AC_TASK_DISPLAY_ID` | Screen manager dispatch and rendering | Current screen pointer through screen manager | Display button signals, startup signals, game tick |
| `scr_startup.cpp` | Logo and menu state | `startup_show_logo`, `startup_menu_index` | `AC_DISPLAY_INITIAL`, button signals, logo timer |
| `scr_game.cpp` | Gameplay state and object model | Player state, vehicle pool, score, level, game_over | `SCREEN_ENTRY`, game tick, button signals |
| `app_bsp.cpp` | Button event translation | Button driver objects | Hardware button state from button driver |
| `screen_manager.cpp` | Current screen and render throttle | Current/old screen handler and view screen | Messages from `task_display()` |
| `timer.c` | Periodic and one-shot software timers | Timer pool | `timer_set()`, `timer_remove_attr()` |

Important difference from multi-object-task games:

```text
Reverse Lane:
    screen owns gameplay objects directly.

In The Depth style:
    each object can own a task and receive object-specific signals.
```

Reverse Lane is intentionally simpler because it is a smaller game.

---

## X. Code References

| Area | File |
|---|---|
| Task IDs and task registration | `application/sources/app/task_list.h`, `task_list.cpp` |
| Display signal definitions | `application/sources/app/app.h` |
| Button callback logic | `application/sources/app/app_bsp.cpp` |
| Startup logo and menu | `application/sources/app/screens/scr_startup.cpp` |
| Main game screen logic | `application/sources/app/screens/scr_game.cpp` |
| Screen manager | `application/sources/common/screen_manager.cpp` |
| AK task queue | `application/sources/ak/src/task.c` |
| AK timer | `application/sources/ak/src/timer.c` |
| AK message pool | `application/sources/ak/src/message.c` |
| OLED render layer | `application/sources/view/view_render.cpp` |

