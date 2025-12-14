#include "palettes.h"
#include "utils.h"
#include "wasm4.h"

#include <stdbool.h>
#include <string.h>

#define BAR_WIDTH  32
#define BAR_HEIGHT 8
#define BAR_Y      145
#define MIN_BAR_X  1
#define MAX_BAR_X  (SCREEN_SIZE - MIN_BAR_X - BAR_WIDTH)

#define BALL_DIAMETER      4
#define BALL_VELOCITY_DOWN 1
#define BALL_VELOCITY_UP  -1

#define BRICK_INITIAL_X             2
#define BRICK_INITIAL_Y             BALL_DIAMETER + 2
#define BRICK_PAD                   1
#define BRICK_WIDTH_PLUS_PADDING    26
#define BRICK_HEIGHT_PLUS_PADDING   8
#define BRICK_WIDTH                 (BRICK_WIDTH_PLUS_PADDING)  - (BRICK_PAD * 2)
#define BRICK_HEIGHT                (BRICK_HEIGHT_PLUS_PADDING) - (BRICK_PAD * 2)
#define NUM_BRICK_COLS              ((int) ((SCREEN_SIZE - (BRICK_INITIAL_X * 2)) / BRICK_WIDTH_PLUS_PADDING))
#define NUM_BRICK_ROWS              8
#define NUM_BRICKS                  (NUM_BRICK_COLS) * (NUM_BRICK_ROWS)

static char temp_buffer[32];

typedef struct {
    int x;
    int y;
    int width;
    int height;
} Rect;

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
    GAME_OVER_SCREEN,
    NUM_SCREEN
} Screen_Kind;

typedef struct {
    uint8_t health;
    int brick_x;
    int brick_y;
} Brick;

typedef enum {
    LEVEL1,
    LEVEL2,
    LEVEL3,
    LEVEL4,
    LEVEL5,
    LEVEL6,
    LEVEL7,
    LEVEL8,
    NUM_LEVELS
} Level;

typedef enum {
    RIGHT,
    LEFT,
    BOTTOM,
    TOP,
    NUM_DIRECTIONS
} Direction;

typedef struct {
    Screen_Kind screen_kind;

    uint8_t frame_clock;
    uint8_t previous_gamepad;
    Palette_Picker current_palette;

    // Level
    Level level;

    // Lives
    uint8_t num_balls_left;

    // Bar Position
    int bar_x;

    // Ball Position
    int ball_x;
    int ball_y;

    // Ball Velocity
    Ball_Horizontal_Velocity ball_velocity_x;
    int ball_velocity_y;

    Brick bricks[NUM_BRICKS];
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

void update_ball_velocity_based_on_direction(Game_State *state, Direction dir) {
    switch(dir) {
    case TOP:
        state->ball_velocity_y = BALL_VELOCITY_DOWN;
        break;
    case BOTTOM:
        state->ball_velocity_y = BALL_VELOCITY_UP;
        break;
    case LEFT:
        reflect_velocity_x_to_right(state);
        break;
    case RIGHT:
        reflect_velocity_x_to_left(state);
        break;
    case NUM_DIRECTIONS:
    default:
        panicf("Unreachable! Invalid Direction: %d", dir);
        break;
    }
}

// Checks if Other bbox is colliding with Focus bbox
// From Top, Bottom, Left or Right
// Top    -> Other object is above the focus
// Bottom -> Other object is below the focus
// Left   -> Other object is left of the focus
// Right  -> Other object is right of the focus
bool bbox_colliding(Rect focus, Rect other, Direction *direction) {
    if (
        (!overlap(focus.x, focus.x + focus.width,
                  other.x, other.x + other.width)) ||
        (!overlap(focus.y, focus.y + focus.height,
                  other.y, other.y + other.height))
    ) {
        return false;
    }

    if (direction) {
        int overlap_right  = (focus.x + focus.width)  - other.x;
        int overlap_left   = (other.x + other.width)  - focus.x;
        int overlap_bottom = (focus.y + focus.height) - other.y;
        int overlap_top    = (other.y + other.height) - focus.y;

        int min_overlap = overlap_left;

        if (overlap_right < min_overlap)  min_overlap = overlap_right;
        if (overlap_top < min_overlap)    min_overlap = overlap_top;
        if (overlap_bottom < min_overlap) min_overlap = overlap_bottom;

        if (min_overlap == overlap_top) {
            *direction = TOP;
        } else if (min_overlap == overlap_bottom) {
            *direction = BOTTOM;
        } else if (min_overlap == overlap_left) {
            *direction = LEFT;
        } else {
            *direction = RIGHT;
        }
    }

    return true;
}

int count_alive_bricks(const Game_State *state) {
    int count = 0;
    for (int i = 0; i < NUM_BRICKS; i++) {
        if (state->bricks[i].health > 0) {
            count++;
        }
    }
    return count;
}

bool any_brick_alive(const Game_State *state) {
    bool any_brick_alive = false;
    for (int i = 0; i < NUM_BRICKS; i++) {
        if (state->bricks[i].health > 0) {
            any_brick_alive = true;
            break;
        }
    }
    return any_brick_alive;
}

void reset_ball(Game_State *state) {
    state->ball_x = state->bar_x + (BAR_WIDTH >> 1) - (BALL_DIAMETER >> 1);
    state->ball_y = BAR_Y - BALL_DIAMETER;
    state->ball_velocity_x.kind = BHV_N_N;
    state->ball_velocity_x.mode = false;
    state->ball_velocity_y = 0;
}

void reset_bricks(Game_State *state) {
    for (int i = 0; i < NUM_BRICKS; i++) {
        int x = i % NUM_BRICK_COLS;
        int y = i / NUM_BRICK_COLS;
        switch(state->level) {
        case LEVEL1:
            state->bricks[i].health = 1;
            break;
        case LEVEL2:
            state->bricks[i].health = 2;
            break;
        case LEVEL3:
            state->bricks[i].health = 3;
            break;
        case LEVEL4:
            state->bricks[i].health = 4;
            break;
        case LEVEL5:
            state->bricks[i].health = 5;
            break;
        case LEVEL6:
            state->bricks[i].health = 6;
            break;
        case LEVEL7:
            state->bricks[i].health = 7;
            break;
        case LEVEL8:
            state->bricks[i].health = 8;
            break;
        case NUM_LEVELS:
        default:
            panicf("Unreachable! Invalid level: %d", state->level);
            break;
        }
        state->bricks[i].brick_x = BRICK_PAD + BRICK_INITIAL_X + x * BRICK_WIDTH_PLUS_PADDING;
        state->bricks[i].brick_y = BRICK_PAD + BRICK_INITIAL_Y + y * BRICK_HEIGHT_PLUS_PADDING;
    }
}

void reset_level(Game_State *state) {
    state->num_balls_left = 3;
    state->bar_x = MIN_BAR_X;
    reset_ball(state);
    reset_bricks(state);
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
    state.screen_kind = HELP_SCREEN;
    // state.screen_kind = GAME_SCREEN;
    // state.screen_kind = GAME_OVER_SCREEN;
    state.frame_clock = 0;
    state.current_palette = FROGGYOS;
    state.level = LEVEL1;
    reset_level(&state);
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
    if (pressed_this_frame & BUTTON_UP) {
        state.screen_kind = HELP_SCREEN;
    }
    if (state.screen_kind == GAME_OVER_SCREEN) {
        if (pressed_this_frame & BUTTON_DOWN) {
            if (!any_brick_alive(&state)) {
                state.level = (state.level + 1) % NUM_LEVELS;
            }
            reset_level(&state);
            state.screen_kind = GAME_SCREEN;
            return;
        }
    } else {
        if (pressed_this_frame & BUTTON_DOWN) {
            state.screen_kind = GAME_SCREEN;
        }
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

        text_y += FONT_SIZE + text_ypad;
        text_y += FONT_SIZE + text_ypad;

        {
            *DRAW_COLORS = 0x03;
            text("Click on x", text_x, text_y);

            text_y += FONT_SIZE + text_ypad;
            text("to switch the color", text_x, text_y);

            text_y += FONT_SIZE + text_ypad;
            text("palette.", text_x, text_y);
        }

        text_y += FONT_SIZE + text_ypad;
        text_y += FONT_SIZE + text_ypad;

        {
            *DRAW_COLORS = 0x01;
            text("Press down arrow to", text_x, text_y);

            text_y += FONT_SIZE + text_ypad;
            text("start the game!", text_x, text_y);
        }

        text_y += FONT_SIZE + text_ypad;
        text_y += FONT_SIZE + text_ypad;

        {
            *DRAW_COLORS = 0x03;
            text("Press up arrow", text_x, text_y);

            text_y += FONT_SIZE + text_ypad;
            text("in game to open", text_x, text_y);

            text_y += FONT_SIZE + text_ypad;
            text("this help", text_x, text_y);
        }

        break;
    }
    case GAME_SCREEN: {
        if (!any_brick_alive(&state)) {
            state.screen_kind = GAME_OVER_SCREEN;
            return;
        }

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
            Rect ball_bbox = {
                .x=state.ball_x,
                .y=state.ball_y,
                .width=BALL_DIAMETER,
                .height=BALL_DIAMETER,
            };
            Rect bar_bbox = {
                .x=state.bar_x,
                .y=BAR_Y,
                .width=BAR_WIDTH,
                .height=BAR_HEIGHT,
            };
            if (state.ball_y <= 0 && state.ball_velocity_y != 0) {
                // Upper Wall
                state.ball_velocity_y = BALL_VELOCITY_DOWN;
            } else if (state.ball_y + BALL_DIAMETER >= SCREEN_SIZE - 1) {
                // Lower Wall
                if (state.num_balls_left > 0) {
                    reset_ball(&state);
                    state.num_balls_left--;
                } else {
                    state.screen_kind = GAME_OVER_SCREEN;
                    return;
                }
            }
            if (state.ball_x <= 0) {
                // Left Wall
                reflect_velocity_x_to_right(&state);
            } else if (state.ball_x + BALL_DIAMETER >= SCREEN_SIZE) {
                // Right Wall
                reflect_velocity_x_to_left(&state);
            }

            if (state.ball_velocity_y != 0) {
                bool colliding = false;
                Direction dir;
                if (bbox_colliding(ball_bbox, bar_bbox, &dir)) {
                    update_ball_velocity_based_on_direction(&state, dir);
                    colliding = true;
                }
                if (colliding) {
                    if (gamepad & BUTTON_LEFT) {
                        update_ball_velocity_x_to_left(&state);
                    }
                    if (gamepad & BUTTON_RIGHT) {
                        update_ball_velocity_x_to_right(&state);
                    }
                }
            }

            for (int i = 0; i < NUM_BRICKS; i++) {
                if (state.bricks[i].health == 0) continue;
                Rect brick_bbox = {
                    .x=state.bricks[i].brick_x,
                    .y=state.bricks[i].brick_y,
                    .width=BRICK_WIDTH,
                    .height=BRICK_HEIGHT,
                };
                bool colliding = false;
                Direction dir;
                if (bbox_colliding(ball_bbox, brick_bbox, &dir)) {
                    update_ball_velocity_based_on_direction(&state, dir);
                    colliding = true;
                }
                if (colliding) {
                    state.bricks[i].health--;
                    // 262 Hz - 523 Hz
                    // 30 frames i.e; 0.5 sec
                    // 100% volume
                    // TONE_PULSE1
                    // tone (262, 30, 100, TONE_PULSE1);
                    // tone(262, 60, 100, TONE_PULSE1 | TONE_MODE3);
                    tone(262 | (523 << 16), 5, 25, TONE_PULSE1 | TONE_MODE1);
                    break;
                }
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
                panicf("Invalid state.ball_velocity_x.kind: %d", state.ball_velocity_x.kind);
            }
        }

        // Draw
        {
            *DRAW_COLORS = 0x43;
            for (uint8_t i = 0; i < state.num_balls_left; i++) {
                rect(1 + i * (BALL_DIAMETER + 1), 1, BALL_DIAMETER, BALL_DIAMETER);
            }
            rect(state.ball_x, state.ball_y, BALL_DIAMETER, BALL_DIAMETER);

            *DRAW_COLORS = 0x41;
            rect(state.bar_x, BAR_Y, BAR_WIDTH, BAR_HEIGHT);

            for (int i = 0; i < NUM_BRICKS; i++) {
                if (state.bricks[i].health <= 0) {
                    continue;
                }
                *DRAW_COLORS = 0x03;
                rect(state.bricks[i].brick_x,
                     state.bricks[i].brick_y,
                     BRICK_WIDTH,
                     BRICK_HEIGHT);
                *DRAW_COLORS = 0x04;
                for (int j = 0; j < state.bricks[i].health; j++) {
                    vline(state.bricks[i].brick_x + j,
                          state.bricks[i].brick_y,
                          BRICK_HEIGHT);
                }
            }
        }
        break;
    }
    case GAME_OVER_SCREEN: {
        int text_x = 5;
        int text_y = 5;
        int text_ypad = 3;

        {
            int count = count_alive_bricks(&state);
            if (count > 0) {
                {
                    *DRAW_COLORS = 0x04;
                    text("Game Over :(", text_x, text_y);

                    text_y += FONT_SIZE + text_ypad;
                    text("You have destroyed", text_x, text_y);

                    itoa(NUM_BRICKS - count, temp_buffer, 10);
                    text_y += FONT_SIZE + text_ypad;
                    text(temp_buffer, text_x, text_y);
                    text("bricks",
                         text_x + (FONT_SIZE * (1 + (int) strlen(temp_buffer))),
                         text_y);
                }

                text_y += FONT_SIZE + text_ypad;
                text_y += FONT_SIZE + text_ypad;

                {
                    *DRAW_COLORS = 0x01;
                    text("Press down arrow to", text_x, text_y);

                    text_y += FONT_SIZE + text_ypad;
                    text("retry!", text_x, text_y);
                }
            } else {
                {
                    *DRAW_COLORS = 0x04;
                    text("Congratulations!", text_x, text_y);

                    text_y += FONT_SIZE + text_ypad;
                    text("You have destroyed", text_x, text_y);

                    text_y += FONT_SIZE + text_ypad;
                    text("All the bricks!", text_x, text_y);
                }

                text_y += FONT_SIZE + text_ypad;
                text_y += FONT_SIZE + text_ypad;

                {
                    *DRAW_COLORS = 0x01;
                    text("Press down arrow to", text_x, text_y);

                    if (state.level + 1 < NUM_LEVELS) {
                        text_y += FONT_SIZE + text_ypad;
                        text("next level!", text_x, text_y);
                    } else {
                        text_y += FONT_SIZE + text_ypad;
                        text("restart from", text_x, text_y);
                        text_y += FONT_SIZE + text_ypad;
                        text("the beginning!", text_x, text_y);
                    }
                }
            }
        }

        break;
    }
    case NUM_SCREEN:
    default:
        panic("Unreachable!");
    }

    state.frame_clock = (state.frame_clock + 1) % 60;
    state.previous_gamepad = gamepad;
}
