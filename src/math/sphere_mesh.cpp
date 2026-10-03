/**
 * @file src/math/sphere_mesh.cpp
 * The subdivided-octahedron sphere mesh and the 1026-entry direction table.
 * The original author notes and decompiles are in docs/original/math/.
 */

#include "halo/math/math.hpp"
#include "halo/math/math_globals.h"

#include "win32.h"
#include "tags.h"


namespace halo::math {

sphere_mesh * sphere_mesh_generate(int16_t subdivisions)
{
    sphere_mesh *mesh;
    sphere_mesh_edge_cache *edge_cache;
    int16_t point_count;
    int16_t next_point_index;
    int16_t strip_cursor;
    int i;

    mesh = (sphere_mesh *)GlobalAlloc(0, sizeof(sphere_mesh));
    if (mesh == 0) {
        return 0;
    }

    mesh->triangle_count = (int16_t)(subdivisions * subdivisions * 8);
    point_count = (int16_t)((int16_t)(((subdivisions - 2) * (subdivisions - 1) * 8) / 2) - 6 +
                             subdivisions * 12);
    mesh->point_count = point_count;
    mesh->subdivisions = subdivisions;

    mesh->points = (real_point3d *)GlobalAlloc(0, (uint32_t)(point_count * (int)sizeof(real_point3d)));
    mesh->indices = (int16_t *)GlobalAlloc(0, (uint32_t)((int)mesh->triangle_count << 3));
    mesh->strip_count = 0;

    edge_cache = (sphere_mesh_edge_cache *)GlobalAlloc(0, sizeof(sphere_mesh_edge_cache));

    if (mesh->points != 0) {
        if (mesh->indices != 0 && edge_cache != 0) {
            int row, col;
            for (row = 0; row < 8; row++) {
                for (col = 0; col < 8; col++) {
                    edge_cache->point_index[row][col] = -1;
                }
            }

            for (i = 0; i < 6; i++) {
                mesh->points[i] = k_octahedron_vertices[i];
            }

            strip_cursor = 0;
            next_point_index = 6;
            for (i = 0; i < 8; i++) {
                sphere_mesh_build_face(&next_point_index, mesh,
                                        k_octahedron_faces[i][0], k_octahedron_faces[i][1],
                                        k_octahedron_faces[i][2], strip_cursor, edge_cache);
            }
            GlobalFree(edge_cache);
            return mesh;
        }
        GlobalFree(mesh->points);
    }
    if (mesh->indices != 0) {
        GlobalFree(mesh->indices);
    }
    if (edge_cache != 0) {
        GlobalFree(edge_cache);
    }
    return mesh;
}

void sphere_mesh_build_face(int16_t *next_point_index, sphere_mesh *mesh, int16_t vertex_a, int16_t vertex_b, int16_t apex, int16_t &strip_cursor, sphere_mesh_edge_cache *edge_cache)
{
    sphere_mesh_face_cache *face_cache;
    int32_t entry_count;
    int32_t i;
    int16_t row;
    int16_t col;
    int16_t strip_length;
    int16_t tl, bl, br, tr;

    entry_count = (int16_t)((mesh->subdivisions + 1) * (mesh->subdivisions + 1));
    face_cache = (sphere_mesh_face_cache *)GlobalAlloc(0, (uint32_t)(entry_count * 2));
    if (face_cache != 0) {
        if (0 < entry_count) {
            for (i = 0; i < entry_count; i++) {
                face_cache->point_index[i] = -1;
            }
        }
        row = 1;
        if (0 < mesh->subdivisions) {
            strip_length = 3;
            do {
                mesh->indices[strip_cursor] = strip_length;
                strip_cursor = strip_cursor + 1;
                mesh->strip_count = mesh->strip_count + 1;
                col = 1;
                if (2 < strip_length) {
                    do {
                        tl = sphere_mesh_get_face_point(mesh, next_point_index, apex, vertex_a, vertex_b,
                                                         (int16_t)(row - 1), (int16_t)(col - 1), edge_cache, *face_cache);
                        bl = sphere_mesh_get_face_point(mesh, next_point_index, apex, vertex_a, vertex_b,
                                                         row, (int16_t)(col - 1), edge_cache, *face_cache);
                        br = sphere_mesh_get_face_point(mesh, next_point_index, apex, vertex_a, vertex_b,
                                                         row, col, edge_cache, *face_cache);
                        if (col == 1) {
                            mesh->indices[strip_cursor] = bl;
                            strip_cursor = strip_cursor + 1;
                            mesh->indices[strip_cursor] = tl;
                            strip_cursor = strip_cursor + 1;
                        }
                        mesh->indices[strip_cursor] = br;
                        strip_cursor = strip_cursor + 1;
                        if (col < row) {
                            tr = sphere_mesh_get_face_point(mesh, next_point_index, apex, vertex_a, vertex_b,
                                                             (int16_t)(row - 1), col, edge_cache, *face_cache);
                            mesh->indices[strip_cursor] = tr;
                            strip_cursor = strip_cursor + 1;
                        }
                        col = col + 1;
                    } while (col <= row);
                }
                row = row + 1;
                strip_length = strip_length + 2;
            } while (row <= mesh->subdivisions);
        }
        GlobalFree(face_cache);
    }
}

int16_t sphere_mesh_get_face_point(sphere_mesh *mesh, int16_t *next_point_index, int16_t apex, int16_t vertex_a, int16_t vertex_b, int16_t row, int16_t col, sphere_mesh_edge_cache *edge_cache, sphere_mesh_face_cache &face_cache)
{
    int32_t cache_index;
    int16_t *cached;
    int16_t new_index;
    int16_t left;
    int16_t right;

    cache_index = (mesh->subdivisions + 1) * row + col;
    cached = &face_cache.point_index[cache_index];
    if (*cached == -1) {
        if (col == 0) {
            *cached = sphere_mesh_get_edge_point(apex, vertex_a, mesh, row, *next_point_index, *edge_cache);
        } else if (row == mesh->subdivisions) {
            *cached = sphere_mesh_get_edge_point(vertex_a, vertex_b, mesh, col, *next_point_index, *edge_cache);
        } else if (col == row) {
            *cached = sphere_mesh_get_edge_point(apex, vertex_b, mesh, row, *next_point_index, *edge_cache);
        } else {
            new_index = *next_point_index;
            *next_point_index = new_index + 1;
            left = sphere_mesh_get_edge_point(apex, vertex_a, mesh, row, *next_point_index, *edge_cache);
            right = sphere_mesh_get_edge_point(apex, vertex_b, mesh, row, *next_point_index, *edge_cache);
            *cached = new_index;
            sphere_mesh_interpolate_vertex(col, row, *mesh, new_index, left, right);
        }
    }
    return *cached;
}

int16_t sphere_mesh_get_edge_point(int16_t vertex_a, int16_t vertex_b, sphere_mesh *mesh, int16_t position, int16_t &next_point_index, sphere_mesh_edge_cache &edge_cache)
{
    int16_t hi;
    int16_t lo;
    int16_t *cached;
    int16_t step;
    int16_t new_index;

    hi = vertex_a;
    lo = vertex_b;
    if (vertex_a <= vertex_b) {
        hi = vertex_b;
        lo = vertex_a;
    }
    if (position == 0) {
        return vertex_a;
    }
    if (position == mesh->subdivisions) {
        return vertex_b;
    }
    cached = &edge_cache.point_index[lo][hi];
    if (*cached == -1) {
        *cached = next_point_index;
        if (1 < mesh->subdivisions) {
            for (step = 1; step < mesh->subdivisions; step++) {
                new_index = next_point_index;
                next_point_index = new_index + 1;
                sphere_mesh_interpolate_vertex(step, mesh->subdivisions, *mesh, new_index, lo, hi);
            }
        }
    }
    if (vertex_b < vertex_a) {
        return (int16_t)((uint16_t)(*cached + (mesh->subdivisions - position)) - 1);
    }
    return (int16_t)((uint16_t)(*cached + position) - 1);
}

void sphere_mesh_interpolate_vertex(int16_t position, int16_t total, sphere_mesh &mesh, int16_t new_index, int16_t vertex_lo, int16_t vertex_hi)
{
    real t;
    real one_minus_t;
    real_point3d *lo;
    real_point3d *hi;
    real_point3d *out;

    t = (real)position / (real)total;
    one_minus_t = 1.0f - t;
    hi = &mesh.points[vertex_hi];
    lo = &mesh.points[vertex_lo];
    out = &mesh.points[new_index];

    out->x = t * hi->x + one_minus_t * lo->x;
    out->y = one_minus_t * lo->y + t * hi->y;
    out->z = one_minus_t * lo->z + t * hi->z;
    vector3d_normalize_with_length(*((real_vector3d *)out));
}

void sphere_point_table_init()
{
    sphere_mesh *mesh;
    real_point3d *points;
    int16_t i;

    effect_random_seed = random_seed_generate();
    mesh = sphere_mesh_generate(k_sphere_point_table_subdivisions);

    points = (real_point3d *)GlobalAlloc(0, (uint32_t)mesh->point_count * sizeof(real_point3d));
    sphere_point_table_count = mesh->point_count;
    sphere_point_table = points;

    for (i = 0; i < sphere_point_table_count; i++) {
        points[i] = mesh->points[i];
    }

    GlobalFree(mesh->points);
    GlobalFree(mesh->indices);
    GlobalFree(mesh);
}

}  // namespace halo::math
