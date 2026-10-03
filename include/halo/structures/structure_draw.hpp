/**
 * @file include/halo/structures/structure_draw.hpp
 * Drawing structure bsp surfaces: leaf face lists, picked polygon and debug draws.
 */
#pragma once

#include "halo/structures/structures_types.hpp"

namespace halo::structures {

/**
 * Gathers bsp surface geometry and draws it, including the picked-polygon and debug surface views.
 */
struct structure_draw {
    /**
     * Locks a geometry buffer for the visible surfaces, fills it with all of them or with the subset named by the surface
     * bits, and submits it to the rasterizer. Returns the geometry handle, or -1 when there was nothing to draw or the
     * lock failed.
     *
     * @address 0x552d60
     */
    static int32_t build_visible_surface_geometry(int32_t *visible_surface_indices, uint32_t *surface_bits, int16_t visible_surface_count);

    /**
     * Copies the vertex and index record of every surface whose bit is set into the output faces in ascending surface
     * order, together with the global surface index.
     *
     * @address 0x552c20
     */
    static void leaf_faces_gather_masked(int32_t *out_surface_indices, uint32_t *surface_bits, ScenarioStructureBSPSurface *out_faces);

    /**
     * Sorts the face index list ascending and copies each named surface's record into the output faces in that order.
     *
     * @address 0x552cf0
     */
    static void leaf_faces_gather_list(int16_t face_count, ScenarioStructureBSPSurface *out_faces, int32_t *face_indices);

    /**
     * Walks every lightmap's materials and invokes the material callback (or the transparent material callback for
     * environment-shaded materials) once per material that overlaps the sorted surface index list, bracketed by the
     * lightmap begin and end callbacks.
     *
     * @address 0x552de0
     */
    static void leaf_faces_for_each(int32_t render_context, structure_lightmap_begin_callback lightmap_begin, structure_material_callback material_cb, structure_lightmap_end_callback lightmap_end, structure_transparent_material_callback transparent_material_cb, int32_t *surface_indices, int16_t surface_index_count);

    /**
     * Debug pass that counts the vertices of a leaf's portals. The result is discarded, as in the original.
     *
     * @address 0x5520b0
     */
    static void leaf_portal_vertex_count_debug(int32_t leaf_index, structure_bsp_leaf_map *leaf_map);

    /**
     * Refreshes the picked polygon geometry handle from the visible surface list, re-runs the debug counting passes when
     * enabled and resets the fog plane vector.
     *
     * @address 0x5527f0
     */
    static void picked_polygon_refresh(void);

    /**
     * Draws the picked polygon geometry through the leaf face walk with the force flag and underwater tint states set,
     * restoring the render flag afterwards.
     *
     * @address 0x5528f0
     */
    static void picked_polygon_draw(void);

    /**
     * Debug draw of the surfaces of the given clusters inside the query sphere.
     *
     * @address 0x552980
     */
    static void debug_draw_surfaces_in_box(void *render_point, real_point3d *query_point, float radius, int16_t cluster_count, int16_t *cluster_indices);

    /**
     * Alternate debug draw of the surfaces of the given clusters inside the query sphere, using the second material
     * callback.
     *
     * @address 0x552a60
     */
    static void debug_draw_surfaces_in_box_alt(void *render_point, real_point3d *query_point, float radius, int16_t cluster_count, int16_t *cluster_indices);

    /**
     * Debug draw of the surfaces inside a query box and planes, gathered with the surface query and drawn with a plain
     * material callback.
     *
     * @address 0x552b40
     */
    static void debug_draw_surfaces_simple(real_point3d *query_point, float radius, real_rectangle3d *query_box, real_plane3d *planes, int16_t plane_count);

};

/**
 * Comparison used to sort face indices ascending: returns whether element is greater than other.
 *
 * @address 0x552c00
 */
uint8_t structure_leaf_face_index_compare(int32_t element, int32_t other);

/**
 * Lightmap begin callback of the picked polygon draw: updates the underwater tint jitter for the bitmap.
 *
 * @address 0x511f20
 */
void structure_picked_polygon_lightmap_begin(void *bitmap_data);

/**
 * Material callback of the picked polygon draw: forwards to the render window's structure material routine.
 *
 * @address 0x511f30
 */
void structure_picked_polygon_material(void *shader_data, int16_t shader_permutation, int32_t render_context, int32_t surface_offset, int16_t surface_count, void *material_extra);

}  // namespace halo::structures
