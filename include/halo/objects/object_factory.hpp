/**
 * @file include/halo/objects/object_factory.hpp
 * Object system API: object factory.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * Creation of objects from placement data and from the scenario, including per-BSP scenario placement.
 */
class ObjectFactory {
public:
    /**
     * Places the objects of a scenario.
     *
     * Original register convention: stack -> scenario.
     *
     * @address 0x004f3ba0
     */
    static void place_scenario(uint8_t *scenario);

    /**
     * Places the scenario objects of a BSP when it is activated.
     *
     * Original register convention: no arguments.
     *
     * @address 0x004f4860
     */
    static void place_for_structure_bsp_on_activate();

    /**
     * Places the scenario objects belonging to the structure BSP.
     *
     * Original register convention: stack -> place.
     *
     * @address 0x004f4880
     */
    static void place_for_structure_bsp(uint8_t place);

    /**
     * Creates an object from placement data and returns its handle.
     *
     * @address 0x004f5460
     */
    static datum_index create(object_placement_data *placement);

    /**
     * Creates an object from placement data with an explicit network role, running the type hooks and network
     * announcements.
     *
     * @address 0x004f54b0
     */
    static datum_index create_with_role_control(object_placement_data *placement, uint32_t role);

    /**
     * Creates the object registered under a scenario name.
     *
     * Original register convention: CX -> name_index.
     *
     * @address 0x004f7370
     */
    static datum_index create_from_scenario_name(int16_t name_index);

    /**
     * Returns the handle registered for a scenario object name.
     *
     * Original register convention: index in AX. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f73c5 cmp
     * ax,0x200 at entry, no stack access. // blam-cc: AX -> name_index.
     *
     * @address 0x004f73c0
     */
    static datum_index lookup_by_name(int16_t name_index);

    /**
     * Touches the predicted resources of an object definition tag when it is valid.
     *
     * @address 0x004f7ad0
     */
    static void notify_predicted_resources_if_valid(datum_index definition_tag);

    /**
     * Creates an object from a scenario placement block using the given palette.
     *
     * Original register convention: EDI -> placement, stack -> palette.
     *
     * @address 0x004f9b70
     */
    static datum_index create_from_scenario_placement(uint8_t *placement, TagReflexive *palette);
};

/**
 * Creation parameters passed to object_new.
 */
class ObjectPlacementDataView {
public:
    explicit ObjectPlacementDataView(object_placement_data *self) : self(self) {}

    /**
     * Initialises placement data for a definition tag and role with default orientation and velocities.
     *
     * @address 0x004f53a0
     */
    void initialize(datum_index definition_tag, datum_index role);

private:
    object_placement_data *self;
};

/**
 * Scenery specific hooks: type initialisation and per-tick update of a scenery object.
 */
class SceneryObject {
public:
    explicit SceneryObject(uint32_t handle) : handle(handle) {}

    /**
     * Type hook run when a scenery object is created.
     *
     * Original register convention: stack -> object_index (cdecl); returns AL.
     *
     * @address 0x004fa7e0
     */
    uint8_t initialize();

    /**
     * Per-tick update of a scenery object.
     *
     * Original register convention: stack -> object_index (cdecl); returns AL.
     *
     * @address 0x004fa870
     */
    uint8_t update();

private:
    uint32_t handle;
};

}  // namespace halo::objects
