#include "palettes.h"
#include "wasm4.h"

#include <stdbool.h>
#include <string.h>

#define BAR_WIDTH 32
#define BAR_HEIGHT 10
#define BAR_Y 145
#define MIN_BAR_X 1
#define MAX_BAR_X (SCREEN_SIZE - MIN_BAR_X - BAR_WIDTH)
#define BALL_DIAMETER 8

#define BALL_VELOCITY_DOWN 1
#define BALL_VELOCITY_UP  -1

typedef enum {
    BHV_L_L, // -1 -1
    BHV_N_L, //  0 -1
    BHV_N_N, //  0  0
    BHV_N_R, //  0  1
    BHV_R_R, //  1  1
    NUM_BHV
} Ball_Horizontal_Velocity_Kind;

typedef struct {
    Ball_Horizontal_Velocity_Kind kind;
    bool mode;
} Ball_Horizontal_Velocity;

typedef enum {
    HELP_SCREEN,
    GAME_SCREEN,
    NUM_SCREEN
} Screen_Kind;

typedef struct {
    Screen_Kind screen_kind;

    size_t frame_clock;
    uint8_t previous_gamepad;
    Palette_Picker current_palette;

    // Bar Position
    int bar_x;

    // Ball Position
    int ball_x;
    int ball_y;

    // Ball Velocity
    Ball_Horizontal_Velocity ball_velocity_x;
    int ball_velocity_y;
} Game_State;

void update_ball_velocity_x_to_left(Game_State *state) {
    switch (state->ball_velocity_x.kind) {
    case BHV_N_L:
        state->ball_velocity_x.kind = BHV_L_L;
        state->ball_velocity_x.mode = false;
        break;
    case BHV_N_N:
        state->ball_velocity_x.kind = BHV_N_L;
        state->ball_velocity_x.mode = false;
        break;
    case BHV_N_R:
        state->ball_velocity_x.kind = BHV_N_N;
        state->ball_velocity_x.mode = false;
        break;
    case BHV_R_R:
        state->ball_velocity_x.kind = BHV_N_R;
        state->ball_velocity_x.mode = false;
        break;
    default:
        break;
    }
}

void update_ball_velocity_x_to_right(Game_State *state) {
    switch (state->ball_velocity_x.kind) {
    case BHV_L_L:
        state->ball_velocity_x.kind = BHV_N_L;
        state->ball_velocity_x.mode = false;
        break;
    case BHV_N_L:
        state->ball_velocity_x.kind = BHV_N_N;
        state->ball_velocity_x.mode = false;
        break;
    case BHV_N_N:
        state->ball_velocity_x.kind = BHV_N_R;
        state->ball_velocity_x.mode = false;
        break;
    case BHV_N_R:
        state->ball_velocity_x.kind = BHV_R_R;
        state->ball_velocity_x.mode = false;
        break;
    default:
        break;
    }
}

void reflect_velocity_x_to_left(Game_State *state) {
    switch (state->ball_velocity_x.kind) {
    case BHV_N_R:
        state->ball_velocity_x.kind = BHV_N_L;
        state->ball_velocity_x.mode = false;
        break;
    case BHV_R_R:
        state->ball_velocity_x.kind = BHV_L_L;
        state->ball_velocity_x.mode = false;
        break;
    default:
        break;
    }
}

void reflect_velocity_x_to_right(Game_State *state) {
    switch (state->ball_velocity_x.kind) {
    case BHV_N_L:
        state->ball_velocity_x.kind = BHV_N_R;
        state->ball_velocity_x.mode = false;
        break;
    case BHV_L_L:
        state->ball_velocity_x.kind = BHV_R_R;
        state->ball_velocity_x.mode = false;
        break;
    default:
        break;
    }
}

int clamp_int(int x, int min_x, int max_x) {
    if (x < min_x) {
        return min_x;
    }
    if (x > max_x) {
        return max_x;
    }
    return x;
}

bool overlap(int l1, int h1, int l2, int h2) {
    return (l1 <= h2) && (l2 <= h1);
}

void reset_ball(Game_State *state) {
    state->ball_x = state->bar_x + (BAR_WIDTH >> 1) - (BALL_DIAMETER >> 1);
    state->ball_y = BAR_Y - BALL_DIAMETER;
    state->ball_velocity_x.kind = BHV_N_N;
    state->ball_velocity_x.mode = false;
    state->ball_velocity_y = 0;
}

Game_State state = {0};

// Clears the background with a particular
// color from the current set palette
// color_index are values from 1-4
// corresponding to 4 colors in the palette
void clear_background() {
    int palette_color = *DRAW_COLORS & 0b1111;
    short color_index = (palette_color - 1) & 0b11;
    memset(FRAMEBUFFER,
           color_index | (color_index << 2) | (color_index << 4) | (color_index << 6),
           SCREEN_SIZE*SCREEN_SIZE/4);
}

void start() {
    // state.screen_kind = HELP_SCREEN;
    state.screen_kind = GAME_SCREEN;
    state.frame_clock = 0;
    state.current_palette = TWO_BIT_DEMICHROME;
    state.bar_x = MIN_BAR_X;
    reset_ball(&state);
    set_palette(state.current_palette);
}

// *DRAW_COLORS = 0xABCD;
// D: Fill Color (0 - 5)
// C: Stroke Color (0 - 5)&
// A and B currently unknown
// ---
// 0: TRANSPARENT
// 1: PALETTE[0] i.e; Color 1
// 2: PALETTE[1] i.e; Color 2
// 3: PALETTE[2] i.e; Color 3
// 4: PALETTE[3] i.e; Color 4

void update() {
    uint8_t gamepad = *GAMEPAD1;
    uint8_t pressed_this_frame = gamepad & (gamepad ^ state.previous_gamepad);

    // Palette Switch
    if (pressed_this_frame & BUTTON_1) {
        state.current_palette = (state.current_palette + 1) % NUM_PALETTE_PICKER;
        set_palette(state.current_palette);
    }
    *DRAW_COLORS = 0x02;
    clear_background();

    // Switch Screen Logic
    if (pressed_this_frame & BUTTON_DOWN) {
        state.screen_kind = GAME_SCREEN;
    } else if (pressed_this_frame & BUTTON_UP) {
        state.screen_kind = HELP_SCREEN;
    }

    switch (state.screen_kind) {
    case HELP_SCREEN: {
        int text_x = 5;
        int text_y = 5;
        int text_ypad = 3;

        {
            *DRAW_COLORS = 0x04;
            text("Welcome to the", text_x, text_y);

            text_y += FONT_SIZE + text_ypad;
            text("Brick Breaker!", text_x, text_y);
        }

        text_y += text_ypad << 2;

        {
            *DRAW_COLORS = 0x03;
            text_y += FONT_SIZE + text_ypad;
            text("Click on x", text_x, text_y);

            text_y += FONT_SIZE + text_ypad;
            text("to switch the color", text_x, text_y);

            text_y += FONT_SIZE + text_ypad;
            text("palette.", text_x, text_y);
        }

        text_y += text_ypad << 2;

        {
            *DRAW_COLORS = 0x01;
            text_y += FONT_SIZE + text_ypad;
            text("Press down arrow", text_x, text_y);

            text_y += FONT_SIZE + text_ypad;
            text("to start the game!", text_x, text_y);
        }

        text_y += text_ypad << 2;

        {
            *DRAW_COLORS = 0x03;
            text_y += FONT_SIZE + text_ypad;
            text("Press up arrow", text_x, text_y);

            text_y += FONT_SIZE + text_ypad;
            text("in game to open", text_x, text_y);

            text_y += FONT_SIZE + text_ypad;
            text("this help", text_x, text_y);
        }

        break;
    }
    case GAME_SCREEN: {
        // Button Actions
        {
            // Bar Movements
            if (gamepad & BUTTON_RIGHT) {
                int next_bar_x = clamp_int(state.bar_x + 1, MIN_BAR_X, MAX_BAR_X);
                if (state.ball_velocity_y == 0 && next_bar_x != state.bar_x) {
                    state.ball_x += 1;
                }
                state.bar_x = next_bar_x;
            }
            if (gamepad & BUTTON_LEFT) {
                int next_bar_x = clamp_int(state.bar_x - 1, MIN_BAR_X, MAX_BAR_X);
                if (state.ball_velocity_y == 0 && next_bar_x != state.bar_x) {
                    state.ball_x -= 1;
                }
                state.bar_x = next_bar_x;
            }
            if ((pressed_this_frame & BUTTON_2) && state.ball_velocity_y == 0) {
                state.ball_velocity_y = BALL_VELOCITY_UP;
            }
        }

        // Animate and State Update
        {
            if (state.ball_y <= 0 && state.ball_velocity_y != 0) {
                // Upper Wall
                state.ball_velocity_y = BALL_VELOCITY_DOWN;
            } else if (state.ball_y + BALL_DIAMETER >= SCREEN_SIZE - 1) {
                // Lower Wall
                reset_ball(&state);
            } else if (overlap(
                           state.ball_x, state.ball_x + BALL_DIAMETER,
                           state.bar_x  , state.bar_x + BAR_WIDTH) &&
                       state.ball_y + BALL_DIAMETER == BAR_Y &&
                       state.ball_velocity_y != 0) {
                // Colliding with bar
                state.ball_velocity_y = BALL_VELOCITY_UP;
                if (gamepad & BUTTON_LEFT) {
                    update_ball_velocity_x_to_left(&state);
                }
                if (gamepad & BUTTON_RIGHT) {
                    update_ball_velocity_x_to_right(&state);
                }
            }

            if (state.ball_x <= 0) {
                // Left Wall
                reflect_velocity_x_to_right(&state);
            } else if (state.ball_x + BALL_DIAMETER >= SCREEN_SIZE) {
                // Right Wall
                reflect_velocity_x_to_left(&state);
            }

            state.ball_y = state.ball_y + state.ball_velocity_y;

            switch (state.ball_velocity_x.kind) {
            case BHV_L_L:
                state.ball_x -= 1;
                state.ball_velocity_x.mode = !state.ball_velocity_x.mode;
                break;
            case BHV_N_L:
                if (state.ball_velocity_x.mode) {
                    state.ball_x -= 1;
                }
                state.ball_velocity_x.mode = !state.ball_velocity_x.mode;
                break;
            case BHV_N_N:
                state.ball_velocity_x.mode = !state.ball_velocity_x.mode;
                break;
            case BHV_N_R:
                if (state.ball_velocity_x.mode) {
                    state.ball_x += 1;
                }
                state.ball_velocity_x.mode = !state.ball_velocity_x.mode;
                break;
            case BHV_R_R:
                state.ball_x += 1;
                state.ball_velocity_x.mode = !state.ball_velocity_x.mode;
                break;
            default:
                tracef("ERROR: Invalid state.ball_velocity_x.kind: %d", state.ball_velocity_x.kind);
            }
        }

        // Draw
        {
            *DRAW_COLORS = 0x43;
            rect(state.ball_x, state.ball_y, BALL_DIAMETER, BALL_DIAMETER);

            *DRAW_COLORS = 0x41;
            rect(state.bar_x, BAR_Y, BAR_WIDTH, BAR_HEIGHT);
        }
        break;
    }
    default:
        trace("Unreachable!");
    }


    state.frame_clock = (state.frame_clock + 1) % 60;
    state.previous_gamepad = gamepad;
}
