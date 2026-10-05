/**
 * @file src/platform/sdl_events.hpp
 * Shared by the SDL platform files: the window's event pump hands every event to the input side as well.
 */
#pragma once

union SDL_Event;

namespace halo::platform {

/** Collects the key and wheel events input_sdl.cpp reports to the engine. */
void sdl_input_event(const SDL_Event &event);

}  // namespace halo::platform
