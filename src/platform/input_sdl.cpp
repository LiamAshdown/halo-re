/**
 * @file src/platform/input_sdl.cpp
 * halo::platform input on SDL2: SDL scancodes as DirectInput scan codes, relative mouse motion while captured, SDL
 * joysticks scaled to the engine's DirectInput data format (range +-0x1000, 10% dead zone, hats as POV angles).
 */

#include "halo/platform/input.hpp"
#include "sdl_events.hpp"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <cstring>

namespace halo::platform {

namespace {

constexpr int k_key_queue_size = 64;
constexpr int32_t k_axis_range = 0x1000;
constexpr int32_t k_dead_zone_percent = 10;
constexpr int32_t k_wheel_notch = 120;

key_event g_keys[k_key_queue_size];
int g_key_head;
int g_key_count;
int32_t g_wheel;
bool g_captured;

/** The DirectInput scan code (DIK_) of an SDL scancode, 0 for keys DirectInput has none for. */
uint8_t directinput_scan_code(SDL_Scancode scancode)
{
    static const uint8_t letters[26] = {0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
                                        0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c};
    static const uint8_t function_keys[12] = {0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x57, 0x58};
    static const uint8_t keypad_digits[9] = {0x4f, 0x50, 0x51, 0x4b, 0x4c, 0x4d, 0x47, 0x48, 0x49};

    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
        return letters[scancode - SDL_SCANCODE_A];
    }
    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_0) {
        return static_cast<uint8_t>(0x02 + (scancode - SDL_SCANCODE_1));  // 1..9 then 0 = 0x02..0x0b
    }
    if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12) {
        return function_keys[scancode - SDL_SCANCODE_F1];
    }
    if (scancode >= SDL_SCANCODE_KP_1 && scancode <= SDL_SCANCODE_KP_9) {
        return keypad_digits[scancode - SDL_SCANCODE_KP_1];
    }
    switch (scancode) {
    case SDL_SCANCODE_ESCAPE: return 0x01;
    case SDL_SCANCODE_MINUS: return 0x0c;
    case SDL_SCANCODE_EQUALS: return 0x0d;
    case SDL_SCANCODE_BACKSPACE: return 0x0e;
    case SDL_SCANCODE_TAB: return 0x0f;
    case SDL_SCANCODE_LEFTBRACKET: return 0x1a;
    case SDL_SCANCODE_RIGHTBRACKET: return 0x1b;
    case SDL_SCANCODE_RETURN: return 0x1c;
    case SDL_SCANCODE_LCTRL: return 0x1d;
    case SDL_SCANCODE_SEMICOLON: return 0x27;
    case SDL_SCANCODE_APOSTROPHE: return 0x28;
    case SDL_SCANCODE_GRAVE: return 0x29;
    case SDL_SCANCODE_LSHIFT: return 0x2a;
    case SDL_SCANCODE_BACKSLASH: return 0x2b;
    case SDL_SCANCODE_NONUSHASH: return 0x2b;
    case SDL_SCANCODE_COMMA: return 0x33;
    case SDL_SCANCODE_PERIOD: return 0x34;
    case SDL_SCANCODE_SLASH: return 0x35;
    case SDL_SCANCODE_RSHIFT: return 0x36;
    case SDL_SCANCODE_KP_MULTIPLY: return 0x37;
    case SDL_SCANCODE_LALT: return 0x38;
    case SDL_SCANCODE_SPACE: return 0x39;
    case SDL_SCANCODE_CAPSLOCK: return 0x3a;
    case SDL_SCANCODE_NUMLOCKCLEAR: return 0x45;
    case SDL_SCANCODE_SCROLLLOCK: return 0x46;
    case SDL_SCANCODE_KP_MINUS: return 0x4a;
    case SDL_SCANCODE_KP_PLUS: return 0x4e;
    case SDL_SCANCODE_KP_0: return 0x52;
    case SDL_SCANCODE_KP_PERIOD: return 0x53;
    case SDL_SCANCODE_NONUSBACKSLASH: return 0x56;
    case SDL_SCANCODE_F13: return 0x64;
    case SDL_SCANCODE_F14: return 0x65;
    case SDL_SCANCODE_F15: return 0x66;
    case SDL_SCANCODE_KP_EQUALS: return 0x8d;
    case SDL_SCANCODE_KP_ENTER: return 0x9c;
    case SDL_SCANCODE_RCTRL: return 0x9d;
    case SDL_SCANCODE_KP_DIVIDE: return 0xb5;
    case SDL_SCANCODE_PRINTSCREEN: return 0xb7;
    case SDL_SCANCODE_RALT: return 0xb8;
    case SDL_SCANCODE_PAUSE: return 0xc5;
    case SDL_SCANCODE_HOME: return 0xc7;
    case SDL_SCANCODE_UP: return 0xc8;
    case SDL_SCANCODE_PAGEUP: return 0xc9;
    case SDL_SCANCODE_LEFT: return 0xcb;
    case SDL_SCANCODE_RIGHT: return 0xcd;
    case SDL_SCANCODE_END: return 0xcf;
    case SDL_SCANCODE_DOWN: return 0xd0;
    case SDL_SCANCODE_PAGEDOWN: return 0xd1;
    case SDL_SCANCODE_INSERT: return 0xd2;
    case SDL_SCANCODE_DELETE: return 0xd3;
    case SDL_SCANCODE_LGUI: return 0xdb;
    case SDL_SCANCODE_RGUI: return 0xdc;
    case SDL_SCANCODE_APPLICATION: return 0xdd;
    default: return 0;
    }
}

int32_t scale_axis(Sint16 raw)
{
    int32_t value = raw < -32767 ? -32767 : raw;
    int32_t magnitude = value < 0 ? -value : value;
    int32_t dead = 32767 * k_dead_zone_percent / 100;

    if (magnitude <= dead) {
        return 0;
    }
    magnitude = (magnitude - dead) * k_axis_range / (32767 - dead);
    return value < 0 ? -magnitude : magnitude;
}

uint32_t pov_angle(Uint8 hat)
{
    switch (hat) {
    case SDL_HAT_UP: return 0;
    case SDL_HAT_RIGHTUP: return 4500;
    case SDL_HAT_RIGHT: return 9000;
    case SDL_HAT_RIGHTDOWN: return 13500;
    case SDL_HAT_DOWN: return 18000;
    case SDL_HAT_LEFTDOWN: return 22500;
    case SDL_HAT_LEFT: return 27000;
    case SDL_HAT_LEFTUP: return 31500;
    default: return 0xffffffff;
    }
}

}  // namespace

void sdl_input_event(const SDL_Event &event)
{
    switch (event.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        if (g_captured && event.key.repeat == 0) {
            uint8_t code = directinput_scan_code(event.key.keysym.scancode);

            if (code != 0 && g_key_count < k_key_queue_size) {
                g_keys[(g_key_head + g_key_count++) % k_key_queue_size] = {code, event.type == SDL_KEYDOWN};
            }
        }
        break;
    case SDL_MOUSEWHEEL:
        if (g_captured) {
            g_wheel += (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -event.wheel.y : event.wheel.y) * k_wheel_notch;
        }
        break;
    default:
        break;
    }
}

bool input_initialize()
{
    SDL_SetMainReady();  // the game has its own WinMain; input may start before the window
    return SDL_InitSubSystem(SDL_INIT_JOYSTICK) == 0;
}

void input_shutdown()
{
    input_set_captured(false);
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

void input_set_captured(bool captured)
{
    g_captured = captured;
    SDL_SetRelativeMouseMode(captured ? SDL_TRUE : SDL_FALSE);
    SDL_GetRelativeMouseState(nullptr, nullptr);  // drop the motion collected before the change
    g_wheel = 0;
}

bool keyboard_next_event(key_event *event)
{
    if (g_key_count == 0) {
        return false;
    }
    *event = g_keys[g_key_head];
    g_key_head = (g_key_head + 1) % k_key_queue_size;
    g_key_count--;
    return true;
}

void keyboard_flush()
{
    g_key_head = 0;
    g_key_count = 0;
}

uint32_t input_modifiers()
{
    SDL_Keymod mod = SDL_GetModState();

    return ((mod & KMOD_SHIFT) != 0 ? k_modifier_shift : 0) | ((mod & KMOD_CTRL) != 0 ? k_modifier_control : 0) |
           ((mod & KMOD_ALT) != 0 ? k_modifier_alt : 0);
}

void mouse_read(mouse_sample *sample)
{
    int x;
    int y;
    Uint32 buttons = SDL_GetRelativeMouseState(&x, &y);

    std::memset(sample, 0, sizeof(*sample));
    sample->x = x;
    sample->y = y;
    sample->wheel = g_wheel;
    g_wheel = 0;
    sample->buttons[0] = (buttons & SDL_BUTTON_LMASK) != 0 ? 0x80 : 0;
    sample->buttons[1] = (buttons & SDL_BUTTON_RMASK) != 0 ? 0x80 : 0;
    sample->buttons[2] = (buttons & SDL_BUTTON_MMASK) != 0 ? 0x80 : 0;
    sample->buttons[3] = (buttons & SDL_BUTTON_X1MASK) != 0 ? 0x80 : 0;
    sample->buttons[4] = (buttons & SDL_BUTTON_X2MASK) != 0 ? 0x80 : 0;
}

int32_t joystick_count()
{
    return SDL_NumJoysticks();
}

void *joystick_open(int32_t index, joystick_info *info)
{
    SDL_Joystick *joystick = SDL_JoystickOpen(index);
    SDL_JoystickGUID guid;
    const char *name;

    if (joystick == nullptr) {
        return nullptr;
    }
    guid = SDL_JoystickGetGUID(joystick);
    std::memcpy(info->guid, guid.data, sizeof(info->guid));
    name = SDL_JoystickName(joystick);
    std::strncpy(info->name, name != nullptr ? name : "Joystick", sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = 0;
    info->axis_count = SDL_JoystickNumAxes(joystick);
    info->button_count = SDL_JoystickNumButtons(joystick);
    info->pov_count = SDL_JoystickNumHats(joystick);
    return joystick;
}

void joystick_close(void *joystick)
{
    SDL_JoystickClose(static_cast<SDL_Joystick *>(joystick));
}

bool joystick_read(void *joystick, joystick_sample *sample)
{
    SDL_Joystick *device = static_cast<SDL_Joystick *>(joystick);
    int count;
    int i;

    if (SDL_JoystickGetAttached(device) == SDL_FALSE) {
        return false;
    }
    SDL_JoystickUpdate();
    std::memset(sample, 0, sizeof(*sample));
    count = SDL_JoystickNumAxes(device);
    for (i = 0; i < count && i < 0x20; i++) {
        sample->axes[i] = scale_axis(SDL_JoystickGetAxis(device, i));
    }
    for (i = 0; i < 0x10; i++) {
        sample->povs[i] = 0xffffffff;
    }
    count = SDL_JoystickNumHats(device);
    for (i = 0; i < count && i < 0x10; i++) {
        sample->povs[i] = pov_angle(SDL_JoystickGetHat(device, i));
    }
    count = SDL_JoystickNumButtons(device);
    for (i = 0; i < count && i < 0x20; i++) {
        sample->buttons[i] = SDL_JoystickGetButton(device, i) != 0 ? 0x80 : 0;
    }
    return true;
}

}  // namespace halo::platform
