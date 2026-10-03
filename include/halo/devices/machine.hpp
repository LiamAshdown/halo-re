#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "devices.h"
#include "game.h"
#include "units.h"

namespace halo::devices {
namespace {

/**
 * View of one machine device (doors, elevators, power-driven machinery) addressed by its
 * object handle: creation, placement, melee reaction and the per-tick update.
 */
class MachineHandle {
public:
    explicit MachineHandle(datum_index value) : handle(value) {}
    uint8_t create();
    void place(uint8_t *placement);
    void melee_attacked();
    uint32_t update();

    datum_index handle;
};

/**
 * View of one control device (switches and panels) addressed by its object handle: placement
 * and the touched/activated handling.
 */
class ControlHandle {
public:
    explicit ControlHandle(datum_index value) : handle(value) {}
    void place(uint8_t *placement);
    void activate();
    void touched();

    datum_index handle;
};

/**
 * View of one light fixture device addressed by its object handle.
 */
class LightFixtureHandle {
public:
    explicit LightFixtureHandle(datum_index value) : handle(value) {}
    void place(uint8_t *placement);

    datum_index handle;
};

}
}
