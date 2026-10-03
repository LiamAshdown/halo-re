/**
 * @file include/halo/objects/flag.hpp
 * Object system API: flag.
 * The C symbols other modules link against are the wrappers in src/objects/objects_c_api.cpp.
 */
#pragma once

#include "halo/objects/engine_types.hpp"

namespace halo::objects {

/**
 * Flag widgets: datum table management, per-tick update and rendering.
 */
class FlagSystem {
public:
    /**
     * Creates the flag data array with room for two flags.
     *
     * Original register convention: none (no parameters).
     *
     * @address 0x004fb4d0
     */
    static void initialize();

    /**
     * Disposes the flag data array.
     *
     * Original register convention: none (no parameters).
     *
     * @address 0x004fb4f0
     */
    static void dispose();

    /**
     * Clears the disposing flag of the flag data array after a dispose pass.
     *
     * Original register convention: none (no parameters).
     *
     * @address 0x004fb510
     */
    static void clear_disposing_flag();

    /**
     * Re-reads the flag data array pointer from the game state after a state restore.
     *
     * Original register convention: none (no parameters).
     *
     * @address 0x004fb520
     */
    static void reset_data_pointer();

    /**
     * Allocates a flag datum for a Flag tag and returns its handle; refuses grids with too many vertices.
     *
     * Original register convention: stack -> flag_tag.
     *
     * @address 0x004fb540
     */
    static datum_index create(datum_index flag_tag);

    /**
     * Frees a flag datum from the flag data array.
     *
     * Original register convention: stack -> flag_index (cdecl).
     *
     * @address 0x004fb970
     */
    static void destroy(datum_index flag_index);

    /**
     * Widget render hook for a flag: resolves the owning object and the flag datum.
     *
     * Original register convention: stack -> object_index, flag_index, arg3, arg4 (cdecl).
     *
     * @address 0x004fb980
     */
    static void render_callback(datum_index object_index, datum_index flag_index, uint32_t arg3, uint32_t arg4);

    /**
     * Runs the per-tick update of every live flag.
     *
     * Original register convention: single float stack parameter (dt); Ghidra shows a clean param_1 with no in_REG
     * marker.
     *
     * @address 0x004fba00
     */
    static void update(float dt);

    /**
     * Builds the triangle mesh of one flag's cloth grid (vertices with normals and texture coordinates, triangles
     * from the per-cell split codes) in the dynamic vertex and index caches and draws it with the flag's red or
     * blue shader.
     *
     * Original register convention: EAX -> tag; stack -> entry, lighting, animation (the two change-colour and
     * function-value table pointers); cdecl caller cleanup.
     *
     * @address 0x004fc350
     */
    static void render(Flag *tag, flag *entry, const render_lighting *lighting, const uint32_t *animation);
};

/**
 * One flag widget instance: a cloth grid anchored to the pole markers.
 */
class FlagView {
public:
    explicit FlagView(flag *self) : self(self) {}

    /**
     * Marks the border cells of the flag's cloth grid so the simulation treats them specially.
     *
     * Original register convention: EDI -> tag, carried over unmodified from flag_new's own frame (this function
     * never reloads it); stack -> entry (the one value flag_new actually pushes).
     *
     * @address 0x004fb6d0
     */
    void cloth_mark_border_cells(Flag *tag);

    /**
     * Initialises the rest lengths and shape constraints of the flag's cloth grid from the Flag tag.
     *
     * Original register convention: EDI -> tag; stack -> entry.
     *
     * @address 0x004fb770
     */
    void cloth_init_shape_constraints(Flag *tag);

    /**
     * Writes the split code into a rectangular region of the flag's per-cell split flag table.
     *
     * Original register convention: EAX -> outer_start (Ghidra's in_AX), stack -> tag, entry, inner_start, size,
     * split_code (in that order, matching both call sites' push order).
     *
     * @address 0x004fb840
     */
    void cloth_stamp_region_split_flags(int16_t outer_start, Flag *tag, int16_t inner_start, int16_t size,
    uint16_t split_code);

    /**
     * Advances the flag cloth simulation by dt seconds.
     *
     * Original register convention: three clean stack parameters (entry, tag data, dt); Ghidra shows no in_REG/unaff_
     * markers for this function's own parameters (unlike its callees).
     *
     * @address 0x004fbae0
     */
    void cloth_update(Flag *tag, float dt);

    /**
     * Gathers the pole marker positions that anchor the flag's cloth columns and rows.
     *
     * @address 0x004fc020
     */
    void pole_get_marker_positions(bsp_leaf_reference *node_ref, real_point3d *marker_positions, uint8_t *row_table,
    int16_t *row_start_scratch, int16_t *column_marker_index, Flag *tag);

private:
    flag *self;
};

}  // namespace halo::objects
