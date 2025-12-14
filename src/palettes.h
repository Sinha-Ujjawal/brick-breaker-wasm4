#include "wasm4.h"
#include "utils.h"

#ifndef PALETTES_H_
#define PALETTES_H_

typedef enum {
    ICE_CREAM_GB,
    LAVA_GB,
    TWO_BIT_DEMICHROME,
    ROBOT_ROCK,
    FROGGYOS,
    NUM_PALETTE_PICKER
} Palette_Picker;

// https://lospec.com/palette-list/ice-cream-gb
void palette_ice_cream_gb() {
    PALETTE[0] = 0xfff6d3; // Color 1
    PALETTE[1] = 0xf9a875; // Color 2
    PALETTE[2] = 0xeb6b6f; // Color 3
    PALETTE[3] = 0x7c3f58; // Color 4
}

// https://lospec.com/palette-list/lava-gb
void palette_lava_gb() {
    PALETTE[0] = 0xff8e80; // Color 1
    PALETTE[1] = 0xc53a9d; // Color 2
    PALETTE[2] = 0x4a2480; // Color 3
    PALETTE[3] = 0x051f39; // Color 4
}

// https://lospec.com/palette-list/2bit-demichrome
void palette_2bit_demichrome() {
    PALETTE[0] = 0xe9efec; // Color 1
    PALETTE[1] = 0xa0a08b; // Color 2
    PALETTE[2] = 0x555568; // Color 3
    PALETTE[3] = 0x211e20; // Color 4
}

// https://lospec.com/palette-list/robot-rock
void palette_robot_rock() {
    PALETTE[0] = 0x000000; // Color 1
    PALETTE[1] = 0x462dae; // Color 2
    PALETTE[2] = 0xe89073; // Color 3
    PALETTE[3] = 0xffffff; // Color 4
}

// https://lospec.com/palette-list/froggyos
void palette_froggyos() {
    PALETTE[0] = 0xe2d6b5; // Color 1
    PALETTE[1] = 0x36ad69; // Color 2
    PALETTE[2] = 0x7b7b7b; // Color 3
    PALETTE[3] = 0x343434; // Color 4
}

void set_palette(Palette_Picker palette) {
    switch (palette) {
    case ICE_CREAM_GB:
      palette_ice_cream_gb();
      break;
    case LAVA_GB:
      palette_lava_gb();
      break;
    case TWO_BIT_DEMICHROME:
      palette_2bit_demichrome();
      break;
    case ROBOT_ROCK:
      palette_robot_rock();
      break;
    case FROGGYOS:
      palette_froggyos();
      break;
    default:
      panicf("ERROR: Invalid Color Palette: %d!", palette);
      break;
    }
}

#endif
