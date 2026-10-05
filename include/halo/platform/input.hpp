/**
 * @file include/halo/platform/input.hpp
 * Keyboard, mouse and joysticks, independent of the operating system. Key events carry DirectInput (set 1) scan codes
 * and the mouse and joystick samples have the DirectInput layouts the engine's conversion code reads (DIMOUSESTATE2,
 * its custom joystick format): the encoding the engine's tables and saved bindings use. src/platform/input_sdl.cpp
 * implements this on SDL2; keyboard and mouse events arrive through the window's event pump (window.hpp).
 */
#pragma once

#include <cstdint>

namespace halo::platform {

/** Modifier bits of input_modifiers. */
inline constexpr uint32_t k_modifier_shift = 1;
inline constexpr uint32_t k_modifier_control = 2;
inline constexpr uint32_t k_modifier_alt = 4;

/** One key press or release; scan_code is the DirectInput scan code (DIK_ value). */
struct key_event {
    uint8_t scan_code;
    bool down;
};

/** A mouse sample since the previous one (DIMOUSESTATE2: y down positive, wheel in 120ths of a notch). */
struct mouse_sample {
    int32_t x;
    int32_t y;
    int32_t wheel;
    uint8_t buttons[8];  // left, right, middle, X1, X2; bit 7 while held
};

/** A joystick sample (the engine's DirectInput data format: axes -0x1000..0x1000 after a 10% dead zone,
 * hats in hundredths of a degree clockwise from north or 0xffffffff when centred, buttons bit 7 while held). */
struct joystick_sample {
    int32_t axes[0x20];
    uint32_t povs[0x10];
    uint8_t buttons[0x20];
};

/** What the engine records about a joystick. */
struct joystick_info {
    uint32_t guid[4];
    char name[128];
    int32_t axis_count;
    int32_t button_count;
    int32_t pov_count;
};

/** Starts keyboard, mouse and joystick input. */
bool input_initialize();
void input_shutdown();
/** While captured the mouse is the game's alone (relative motion, no cursor) and key events are collected. */
void input_set_captured(bool captured);

/** The next queued key event; false when there is none. */
bool keyboard_next_event(key_event *event);
/** Drops the queued key events. */
void keyboard_flush();
/** The k_modifier_ bits held now. */
uint32_t input_modifiers();

/** The mouse motion, wheel and buttons since the last read. */
void mouse_read(mouse_sample *sample);

/** The number of joysticks attached, and opening one of them (null on failure). */
int32_t joystick_count();
void *joystick_open(int32_t index, joystick_info *info);
void joystick_close(void *joystick);
bool joystick_read(void *joystick, joystick_sample *sample);

}  // namespace halo::platform
