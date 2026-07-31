#include "scr_game.h"

#include "button.h"
#include "app_bsp.h"

#include <stdlib.h>

#define ROAD_TOP_Y                  (7)
#define ROAD_LANE_COUNT             (4)
#define LANE_HEIGHT                 (14)
#define PLAYER_START_X              (6)
#define PLAYER_RETURN_STEP          (1)
#define PLAYER_SW4_DOUBLE_TAP_TICKS (1)
#define PLAYER_WIDTH                (17)
#define PLAYER_HEIGHT               (18)
#define PLAYER_MIN_X                (0)
#define PLAYER_MAX_X                (LCD_WIDTH - PLAYER_WIDTH)
#define MAX_VEHICLES                (7)
#define GAME_TICK_INTERVAL_MS       (80)
#define PLAYER_JUMP_TICKS           (16)
#define VEHICLE_TYPE_COUNT          (3)

static void view_scr_game();
static void game_init();
static void game_update();
static void game_spawn_vehicle();
static void game_update_difficulty();
static bool game_check_collision();
static bool lane_has_near_vehicle(int lane, int x, int gap);
static void vehicle_try_change_lane(int index);
static int lane_center_y(int lane);
static int level_speed_bonus();
static bool jump_has_clear_speed();
static void player_jump_offset(int* x_offset, int* y_offset);
static void draw_player(int x, int y, bool airborne);

static const unsigned char PROGMEM bitmap_player[] = {
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x1d, 0x98, 0x00,
    0x7f, 0x9e, 0x00,
    0x3e, 0xff, 0x00,
    0x3c, 0xf7, 0x00,
    0x3c, 0xf7, 0x00,
    0x3e, 0xff, 0x00,
    0x7f, 0x9e, 0x00,
    0x0d, 0x98, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00
};

static const unsigned char PROGMEM bitmap_player_large[] = {
    0x3e, 0x00, 0xf8,
    0x7f, 0xff, 0xfc,
    0xff, 0x0f, 0xfe,
    0xbf, 0x07, 0xff,
    0xbf, 0xfc, 0xff,
    0xb9, 0xfc, 0x7f,
    0xf1, 0xdc, 0x7f,
    0xf1, 0xac, 0x5f,
    0xf1, 0xac, 0x5f,
    0xf1, 0xac, 0x5f,
    0xf1, 0xdc, 0x7f,
    0xb9, 0xfc, 0x7f,
    0xbf, 0xfc, 0xff,
    0xbf, 0x07, 0xff,
    0xff, 0x0f, 0xfe,
    0x7f, 0xff, 0xfc,
    0x00, 0x00, 0x00
};

static const unsigned char PROGMEM bitmap_moto[] = {
    0x00, 0x00, 0x00,
    0x7f, 0xf7, 0x00,
    0x7f, 0xe6, 0x80,
    0x6e, 0x04, 0x00,
    0x6e, 0x00, 0x00,
    0x4e, 0x00, 0x00,
    0x6e, 0x06, 0x00,
    0x7f, 0xe6, 0x80,
    0x47, 0xf1, 0x00,
    0x00, 0x00, 0x00
};

static const unsigned char PROGMEM bitmap_f1[] = {
    0x03, 0x80, 0x07, 0x00,
    0x1f, 0xcf, 0xff, 0x00,
    0x1f, 0x8f, 0xff, 0x80,
    0x1f, 0xff, 0xff, 0xc0,
    0x1f, 0xff, 0xff, 0xc0,
    0x1f, 0xff, 0xff, 0xc0,
    0x1b, 0x9f, 0xff, 0xc0,
    0x1f, 0x8f, 0xff, 0x00,
    0x0f, 0x8f, 0xff, 0x00,
    0x01, 0x00, 0x06, 0x00
};

static const unsigned char PROGMEM bitmap_container[] = {
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x1f, 0x7f, 0xff, 0xff, 0xfc,
    0x3e, 0x7f, 0xff, 0xff, 0xfc,
    0x3e, 0xff, 0xff, 0xff, 0xfc,
    0x3e, 0xff, 0xff, 0xff, 0xfc,
    0x3e, 0xff, 0xff, 0xff, 0xfc,
    0x3e, 0xff, 0xff, 0xff, 0xfc,
    0x3e, 0x7f, 0xff, 0xff, 0xfc,
    0x1e, 0x7f, 0xff, 0xff, 0xfc,
    0x0f, 0x07, 0xff, 0xff, 0xfc,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00
};

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

struct game_vehicle_t {
    bool active;
    int lane;
    int x;
    const game_vehicle_type_t* type;
    bool scored;
    int lane_change_counter;
};

static const game_vehicle_type_t vehicle_types[] = {
    {bitmap_moto,      18, 10, 2, 3, 12, true,  true},
    {bitmap_f1,        27, 10, 3, 4, 19, true,  true},
    {bitmap_container, 40, 13, 1, 4, 32, false, false},
};

static game_vehicle_t vehicles[MAX_VEHICLES];
static int player_x = PLAYER_START_X;
static int player_lane = 1;
static int score = 0;
static int level = 1;
static int spawn_counter = 28;
static int spawn_period = 28;
static int player_jump_ticks = 0;
static bool player_returning = false;
static int player_sw4_tap_ticks = 0;
static bool game_over = false;

view_dynamic_t dyn_view_game = {
    {
        .item_type = ITEM_TYPE_DYNAMIC,
    },
    view_scr_game
};

view_screen_t scr_game = {
    &dyn_view_game,
    ITEM_NULL,
    ITEM_NULL,

    .focus_item = 0,
};

static void game_init() {
    player_x = PLAYER_START_X;
    player_lane = 1;
    score = 0;
    level = 1;
    spawn_counter = 28;
    spawn_period = 28;
    player_jump_ticks = 0;
    player_returning = false;
    player_sw4_tap_ticks = 0;
    game_over = false;

    for (int i = 0; i < MAX_VEHICLES; ++i) {
        vehicles[i].active = false;
    }
}

static void game_update() {
    game_update_difficulty();

    if (player_jump_ticks == 0 &&
            !player_returning &&
            player_x > PLAYER_START_X &&
            btn_up.state == BUTTON_SW_STATE_PRESSED &&
            btn_down.state == BUTTON_SW_STATE_PRESSED) {
        player_returning = true;
        player_sw4_tap_ticks = 0;
    }

    if (player_jump_ticks > 0) {
        int phase = PLAYER_JUMP_TICKS - player_jump_ticks;
        static const int jump_auto_dx[PLAYER_JUMP_TICKS] = {1, 2, 2, 2, 3, 3, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0};

        if (phase < 0) {
            phase = 0;
        }
        else if (phase >= PLAYER_JUMP_TICKS) {
            phase = PLAYER_JUMP_TICKS - 1;
        }

        player_x += jump_auto_dx[phase] + (level >= 3 ? 1 : 0);
        if (player_x < PLAYER_MIN_X) {
            player_x = PLAYER_MIN_X;
        }
        else if (player_x > PLAYER_MAX_X) {
            player_x = PLAYER_MAX_X;
        }
    }

    if (player_jump_ticks > 0) {
        --player_jump_ticks;
    }
    else if (player_returning) {
        player_x -= PLAYER_RETURN_STEP;
        if (player_x <= PLAYER_START_X) {
            player_x = PLAYER_START_X;
            player_returning = false;
        }
    }
    else if (player_sw4_tap_ticks > 0) {
        --player_sw4_tap_ticks;
        if (player_sw4_tap_ticks == 0) {
            player_jump_ticks = PLAYER_JUMP_TICKS;
        }
    }

    if (--spawn_counter <= 0) {
        game_spawn_vehicle();
        if (level >= 4 && (rand() % 3) == 0) {
            game_spawn_vehicle();
        }
        spawn_counter = spawn_period + (rand() % (9 - level));
    }

    for (int i = 0; i < MAX_VEHICLES; ++i) {
        if (!vehicles[i].active) {
            continue;
        }

        vehicle_try_change_lane(i);
        vehicles[i].x -= vehicles[i].type->step + level_speed_bonus();

        if (!vehicles[i].scored && vehicles[i].x + vehicles[i].type->width < player_x) {
            vehicles[i].scored = true;
            ++score;
        }

        if (vehicles[i].x + vehicles[i].type->width < 0) {
            vehicles[i].active = false;
        }
    }

    if (game_check_collision()) {
        game_over = true;
        timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_TICK);
    }
}

static void game_spawn_vehicle() {
    for (int slot = 0; slot < MAX_VEHICLES; ++slot) {
        if (vehicles[slot].active) {
            continue;
        }

        int lane = rand() % ROAD_LANE_COUNT;
        if (lane_has_near_vehicle(lane, LCD_WIDTH - 1, 44)) {
            return;
        }

        int type_index = rand() % 2;
        if (level >= 2 && (rand() % 5) == 0) {
            type_index = 2;
        }
        if (level >= 3 && (rand() % 4) == 0) {
            type_index = rand() % VEHICLE_TYPE_COUNT;
        }

        vehicles[slot].active = true;
        vehicles[slot].lane = lane;
        vehicles[slot].x = LCD_WIDTH - 1;
        vehicles[slot].type = &vehicle_types[type_index];
        vehicles[slot].scored = false;
        vehicles[slot].lane_change_counter = 8 + (rand() % 12);
        return;
    }
}

static void game_update_difficulty() {
    if (score >= 100) {
        level = 4;
    }
    else if (score >= 50) {
        level = 3;
    }
    else if (score >= 10) {
        level = 2;
    }
    else {
        level = 1;
    }

    spawn_period = 28 - (level - 1) * 5;
    if (spawn_period < 11) {
        spawn_period = 11;
    }
}

static int level_speed_bonus() {
    switch (level) {
    case 4:
        return 4;
    case 3:
        return 3;
    case 2:
        return 2;
    default:
        return 1;
    }
}

static bool lane_has_near_vehicle(int lane, int x, int gap) {
    for (int i = 0; i < MAX_VEHICLES; ++i) {
        if (!vehicles[i].active || vehicles[i].lane != lane) {
            continue;
        }

        int dx = vehicles[i].x - x;
        if (dx < 0) {
            dx = -dx;
        }

        if (dx < gap) {
            return true;
        }
    }

    return false;
}

static void vehicle_try_change_lane(int index) {
    game_vehicle_t* vehicle = &vehicles[index];
    if (!vehicle->type->can_change_lane || level < 2) {
        return;
    }

    --vehicle->lane_change_counter;
    if (vehicle->lane_change_counter > 0) {
        return;
    }

    vehicle->lane_change_counter = 6 + (rand() % (level >= 4 ? 5 : 10));

    if ((rand() % 100) > 32 + level * 8) {
        return;
    }

    int direction = (rand() & 1) ? 1 : -1;
    int next_lane = vehicle->lane + direction;
    if (next_lane < 0 || next_lane >= ROAD_LANE_COUNT) {
        next_lane = vehicle->lane - direction;
    }

    if (next_lane < 0 || next_lane >= ROAD_LANE_COUNT) {
        return;
    }

    if (!lane_has_near_vehicle(next_lane, vehicle->x, 24)) {
        vehicle->lane = next_lane;
    }
}

static bool game_check_collision() {
    int player_x_offset = 0;
    int player_y_offset = 0;
    player_jump_offset(&player_x_offset, &player_y_offset);

    int player_left = player_x + player_x_offset + 4;
    int player_right = player_left + 11;

    for (int i = 0; i < MAX_VEHICLES; ++i) {
        if (!vehicles[i].active || vehicles[i].lane != player_lane) {
            continue;
        }

        int vehicle_left = vehicles[i].x + vehicles[i].type->hit_x;
        int vehicle_right = vehicle_left + vehicles[i].type->hit_w - 1;

        if (player_left <= vehicle_right && player_right >= vehicle_left) {
            if (player_jump_ticks > 0) {
                if (vehicles[i].type->jumpable || jump_has_clear_speed()) {
                    continue;
                }
            }

            return true;
        }
    }

    return false;
}

static bool jump_has_clear_speed() {
    if (player_jump_ticks <= 0) {
        return false;
    }

    int phase = PLAYER_JUMP_TICKS - player_jump_ticks;
    if (phase < 3 || phase > 11) {
        return false;
    }

    return level >= 3 || player_x >= PLAYER_START_X + 16;
}

static int lane_center_y(int lane) {
    return ROAD_TOP_Y + lane * LANE_HEIGHT + LANE_HEIGHT / 2;
}

static void player_jump_offset(int* x_offset, int* y_offset) {
    *x_offset = 0;
    *y_offset = 0;

    if (player_jump_ticks <= 0) {
        return;
    }

    int phase = PLAYER_JUMP_TICKS - player_jump_ticks;
    static const int jump_y[PLAYER_JUMP_TICKS] = {0, -2, -4, -6, -8, -9, -9, -8, -7, -5, -3, -1, 0, 0, 0, 0};

    if (phase < 0) {
        phase = 0;
    }
    else if (phase >= PLAYER_JUMP_TICKS) {
        phase = PLAYER_JUMP_TICKS - 1;
    }

    *y_offset = jump_y[phase];
}

static void draw_player(int x, int y, bool airborne) {
    if (airborne) {
        int draw_y = y - 2;
        if (draw_y < ROAD_TOP_Y + 1) {
            draw_y = ROAD_TOP_Y + 1;
        }
        if (draw_y > LCD_HEIGHT - 18) {
            draw_y = LCD_HEIGHT - 18;
        }

        view_render.drawBitmap(x, draw_y, bitmap_player_large, 24, 17, WHITE);
    }
    else {
        view_render.drawBitmap(x, y, bitmap_player, PLAYER_WIDTH, PLAYER_HEIGHT, WHITE);
    }
}

static void view_scr_game() {
    view_render.setTextSize(1);
    view_render.setTextColor(WHITE);
    view_render.setCursor(0, 0);
    view_render.print("S:");
    view_render.print(score);
    view_render.setCursor(38, 0);
    view_render.print("L:");
    view_render.print(level);

    for (int i = 0; i <= ROAD_LANE_COUNT; ++i) {
        int y = ROAD_TOP_Y + i * LANE_HEIGHT;
        view_render.drawLine(0, y, LCD_WIDTH - 1, y, WHITE);
    }

    for (int i = 0; i < MAX_VEHICLES; ++i) {
        if (!vehicles[i].active) {
            continue;
        }

        int y = lane_center_y(vehicles[i].lane) - vehicles[i].type->height / 2;
        view_render.drawBitmap(vehicles[i].x, y, vehicles[i].type->bitmap, vehicles[i].type->width, vehicles[i].type->height, WHITE);
    }

    int jump_x = 0;
    int jump_y = 0;
    player_jump_offset(&jump_x, &jump_y);

    int draw_x = player_x + jump_x;
    int player_y = lane_center_y(player_lane) - PLAYER_HEIGHT / 2 + jump_y;
    draw_player(draw_x, player_y, jump_y < 0);

    if (game_over) {
        view_render.fillRect(24, 22, 80, 20, BLACK);
        view_render.drawRect(24, 22, 80, 20, WHITE);
        view_render.setCursor(36, 26);
        view_render.print("GAME OVER");
        view_render.setCursor(22, 44);
        view_render.print("UP/DOWN restart");
    }
}

void scr_game_handle(ak_msg_t* msg) {
    switch (msg->sig) {
    case SCREEN_ENTRY: {
        game_init();
        timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_TICK, GAME_TICK_INTERVAL_MS, TIMER_PERIODIC);
    } break;

    case AC_DISPLAY_GAME_TICK: {
        if (!game_over) {
            game_update();
        }
    } break;

    case AC_DISPLAY_BUTON_UP_PRESSED: {
        if (game_over) {
            game_init();
            timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_TICK, GAME_TICK_INTERVAL_MS, TIMER_PERIODIC);
        }
        else if (player_jump_ticks == 0 && player_lane > 0) {
            player_sw4_tap_ticks = 0;
            --player_lane;
        }
    } break;

    case AC_DISPLAY_BUTON_DOWN_PRESSED: {
        if (game_over) {
            game_init();
            timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_TICK, GAME_TICK_INTERVAL_MS, TIMER_PERIODIC);
        }
        else if (player_jump_ticks == 0 && player_lane < ROAD_LANE_COUNT - 1) {
            player_sw4_tap_ticks = 0;
            ++player_lane;
        }
    } break;

    case AC_DISPLAY_BUTON_MODE_PRESSED: {
        if (game_over) {
            game_init();
            timer_set(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_TICK, GAME_TICK_INTERVAL_MS, TIMER_PERIODIC);
        }
        else if (player_jump_ticks > 0 || player_returning) {
            break;
        }
        else if (player_sw4_tap_ticks > 0 && player_x > PLAYER_START_X) {
            player_returning = true;
            player_sw4_tap_ticks = 0;
        }
        else if (player_x > PLAYER_START_X) {
            player_sw4_tap_ticks = PLAYER_SW4_DOUBLE_TAP_TICKS;
        }
        else {
            player_returning = false;
            player_sw4_tap_ticks = 0;
            player_jump_ticks = PLAYER_JUMP_TICKS;
        }
    } break;

    case AC_DISPLAY_BUTON_MODE_LONG_PRESSED: {
        timer_remove_attr(AC_TASK_DISPLAY_ID, AC_DISPLAY_GAME_TICK);
        SCREEN_TRAN(scr_startup_handle, &scr_startup);
    } break;

    default:
        break;
    }
}