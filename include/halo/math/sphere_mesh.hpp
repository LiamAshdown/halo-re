/**
 * @file include/halo/math/sphere_mesh.hpp
 * The subdivided-octahedron sphere mesh and the 1026-entry direction table.
 * Declared for other modules through halo/math/api.hpp.
 */
#pragma once

#include "halo/math/math_types.hpp"

namespace halo::math {

/**
 * Builds a subdivided octahedron projected onto the unit sphere (GlobalAlloc'd header, points and indices).
 *
 * Original register convention: AX (low half of EAX) -> subdivisions.
 * @address 0x004ca4b0
 */
sphere_mesh * sphere_mesh_generate(int16_t subdivisions);

/**
 * Subdivides one octahedron face into rows of points and run-length triangle strips, sharing edge points
 * through the edge cache.
 *
 * Original register convention: EAX -> next_point_index, ESI -> mesh, stack -> (vertex_a, vertex_b, apex,
 * strip_cursor, edge_cache).
 * @address 0x004ca5f0
 */
void sphere_mesh_build_face(int16_t *next_point_index, sphere_mesh *mesh, int16_t vertex_a, int16_t vertex_b, int16_t apex, int16_t &strip_cursor, sphere_mesh_edge_cache *edge_cache);

/**
 * Returns the point index at (row, col) inside a face, creating and caching it on first use.
 *
 * Original register convention: ECX -> mesh, EBX -> next_point_index, ESI -> apex, stack -> (vertex_a,
 * vertex_b, row, col, edge_cache, face_cache).
 * @address 0x004ca7c0
 */
int16_t sphere_mesh_get_face_point(sphere_mesh *mesh, int16_t *next_point_index, int16_t apex, int16_t vertex_a, int16_t vertex_b, int16_t row, int16_t col, sphere_mesh_edge_cache *edge_cache, sphere_mesh_face_cache &face_cache);

/**
 * Returns the point index at `position` along the edge between two base vertices, creating and caching it on
 * first use.
 *
 * Original register convention: EAX -> vertex_a, ECX -> vertex_b, EDX -> mesh, stack -> (position,
 * next_point_index, edge_cache).
 * @address 0x004ca8c0
 */
int16_t sphere_mesh_get_edge_point(int16_t vertex_a, int16_t vertex_b, sphere_mesh *mesh, int16_t position, int16_t &next_point_index, sphere_mesh_edge_cache &edge_cache);

/**
 * Creates point `new_index` by lerping between two existing points and projecting onto the unit sphere.
 *
 * Original register convention: EAX -> position, ECX -> total, EDI -> mesh, stack -> (new_index, vertex_lo,
 * vertex_hi).
 * @address 0x004ca9a0
 */
void sphere_mesh_interpolate_vertex(int16_t position, int16_t total, sphere_mesh &mesh, int16_t new_index, int16_t vertex_lo, int16_t vertex_hi);

/**
 * Seeds effect_random_seed and builds sphere_point_table: the 1026 points of a 16-subdivision sphere mesh.
 * @address 0x004cd0e0
 */
void sphere_point_table_init();

}  // namespace halo::math
