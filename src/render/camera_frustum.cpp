#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "effects.h"
#include "rasterizer.h"
#include "interface.h"
#include "structures.h"
#include "cutscene.h"
#include "shaders.h"
#include "render.h"
#include <stdint.h>
#include "halo/render/render.hpp"
#include "halo/math/api.hpp"
#include "halo/render/api.hpp"

extern "C" {
extern real_point3d *global_zero_vector3d_pointer;
extern double tan(double x);
extern double fabs(double x);
extern double ftan(double x);
extern uint8_t render_asymmetric_frustum_disabled;
extern float render_camera_global[3];
extern float camera_forward_x[3];
extern float render_camera_facing_basis[16];
extern double sqrt(double x);
extern float render_saved_projection_z[4];
}

/**
 * Fills one side plane of the frustum from the plane normal (i, j, k) and the camera position.
 */
static void build_side_plane(render_frustum *frustum, int16_t index, real i, real j, real k)
{
    real_vector3d normal;
    real_plane3d plane;

    normal.i = i;
    normal.j = j;
    normal.k = k;
    halo::math::vector3d_normalize_with_length(normal);
    plane.normal = normal;
    plane.d = normal.j * global_zero_vector3d_pointer->y + normal.k * global_zero_vector3d_pointer->z +
              normal.i * global_zero_vector3d_pointer->x;
    halo::math::matrix4x3_transform_plane(frustum->world_planes[index], frustum->view_to_world, plane);
}

namespace halo::render::camera {

/**
 * Builds the culling frustum of `camera` (view basis, world planes, far corners, bounds) and,
 * when build_projection is set, its projection matrix and world to screen scale.
 *
 * @address 0x0050cc40
 */
void build_frustum(float *frustum_bounds, render_camera *camera, render_frustum *frustum, uint8_t build_projection)
{
    real width;
    real height;
    real half_width;
    real half_height;
    real cx;
    real cy;
    real t;
    real sx;
    real sy;
    real_vector3d left;
    real_vector3d up;
    real_vector3d backward;
    real_plane3d plane;
    real_point3d point;
    real inverse_sx;
    real inverse_sy;
    real far_x;
    real far_y;
    real mid;
    int16_t i;

    width = (real)((int32_t)camera->viewport_bounds.right - (int32_t)camera->viewport_bounds.left);
    height = (real)((int32_t)camera->viewport_bounds.bottom - (int32_t)camera->viewport_bounds.top);
    if (frustum_bounds != 0) {
        frustum->frustum_bounds[0] = frustum_bounds[0];
        frustum->frustum_bounds[1] = frustum_bounds[1];
        frustum->frustum_bounds[2] = frustum_bounds[2];
        frustum->frustum_bounds[3] = frustum_bounds[3];
    } else {
        frustum->frustum_bounds[2] = -1.0f;
        frustum->frustum_bounds[0] = -1.0f;
        frustum->frustum_bounds[3] = 1.0f;
        frustum->frustum_bounds[1] = 1.0f;
    }
    half_width = (frustum->frustum_bounds[1] - frustum->frustum_bounds[0]) * 0.5f;
    half_height = (frustum->frustum_bounds[3] - frustum->frustum_bounds[2]) * 0.5f;
    cx = (frustum->frustum_bounds[0] + frustum->frustum_bounds[1]) / half_width * -0.5f;
    cy = (frustum->frustum_bounds[2] + frustum->frustum_bounds[3]) / half_height * -0.5f;
    t = (real)tan(camera->vertical_field_of_view * 0.5f);
    sx = 1.0f / (half_width / height * width * t);
    sy = 1.0f / (t * half_height);

    left.i = camera->up.k * camera->forward.j - camera->up.j * camera->forward.k;
    left.j = camera->forward.k * camera->up.i - camera->up.k * camera->forward.i;
    left.k = camera->up.j * camera->forward.i - camera->forward.j * camera->up.i;
    up.i = left.j * camera->forward.k - left.k * camera->forward.j;
    up.j = left.k * camera->forward.i - left.i * camera->forward.k;
    up.k = left.i * camera->forward.j - left.j * camera->forward.i;
    backward.i = -camera->forward.i;
    backward.j = -camera->forward.j;
    backward.k = -camera->forward.k;
    halo::math::vector3d_normalize_with_length(left);
    halo::math::vector3d_normalize_with_length(up);
    halo::math::vector3d_normalize_with_length(backward);
    frustum->view_to_world.forward = left;
    frustum->view_to_world.left = up;
    frustum->view_to_world.up = backward;
    frustum->view_to_world.position = camera->position;
    frustum->view_to_world.scale = 1.0f;
    halo::math::matrix4x3_inverse(&frustum->world_to_view, frustum->view_to_world);

    build_side_plane(frustum, 0, -sx, 0.0f, cx + 1.0f);
    build_side_plane(frustum, 1, sx, 0.0f, 1.0f - cx);
    build_side_plane(frustum, 2, 0.0f, -sy, cy + 1.0f);
    build_side_plane(frustum, 3, 0.0f, sy, 1.0f - cy);
    plane.normal.i = 0.0f;
    plane.normal.j = 0.0f;
    plane.normal.k = 1.0f;
    plane.d = -camera->z_near;
    halo::math::matrix4x3_transform_plane(frustum->world_planes[4], frustum->view_to_world, plane);
    plane.normal.i = 0.0f;
    plane.normal.j = 0.0f;
    plane.normal.k = -1.0f;
    plane.d = camera->z_far;
    halo::math::matrix4x3_transform_plane(frustum->world_planes[5], frustum->view_to_world, plane);
    frustum->z_near = camera->z_near;
    frustum->z_far = camera->z_far;

    inverse_sx = 1.0f / sx;
    far_x = -(inverse_sx * camera->z_far);
    inverse_sy = 1.0f / sy;
    far_y = -(inverse_sy * camera->z_far);
    mid = (camera->z_far + camera->z_near) * 0.5f;
    point.x = (cx + 1.0f) * far_x;
    point.y = (cy + 1.0f) * far_y;
    point.z = -camera->z_far;
    halo::math::matrix4x3_transform_point(frustum->world_vertices[0], point, frustum->view_to_world);
    point.x = (cx - 1.0f) * far_x;
    point.y = (cy + 1.0f) * far_y;
    point.z = -camera->z_far;
    halo::math::matrix4x3_transform_point(frustum->world_vertices[1], point, frustum->view_to_world);
    point.x = (cx + 1.0f) * far_x;
    point.y = (cy - 1.0f) * far_y;
    point.z = -camera->z_far;
    halo::math::matrix4x3_transform_point(frustum->world_vertices[2], point, frustum->view_to_world);
    point.x = (cx - 1.0f) * far_x;
    point.y = (cy - 1.0f) * far_y;
    point.z = -camera->z_far;
    halo::math::matrix4x3_transform_point(frustum->world_vertices[3], point, frustum->view_to_world);
    frustum->world_vertices[4] = camera->position;
    point.x = -(inverse_sx * mid * cx);
    point.y = -(inverse_sy * mid * cy);
    point.z = -mid;
    halo::math::matrix4x3_transform_point(frustum->world_midpoint, point, frustum->view_to_world);

    frustum->world_bounds.x.lower = frustum->world_bounds.x.upper = frustum->world_vertices[0].x;
    frustum->world_bounds.y.lower = frustum->world_bounds.y.upper = frustum->world_vertices[0].y;
    frustum->world_bounds.z.lower = frustum->world_bounds.z.upper = frustum->world_vertices[0].z;
    for (i = 1; i < 5; i++) {
        real_point3d *v = &frustum->world_vertices[i];

        frustum->world_bounds.x.lower = frustum->world_bounds.x.lower <= v->x ? frustum->world_bounds.x.lower : v->x;
        frustum->world_bounds.y.lower = frustum->world_bounds.y.lower <= v->y ? frustum->world_bounds.y.lower : v->y;
        frustum->world_bounds.z.lower = frustum->world_bounds.z.lower <= v->z ? frustum->world_bounds.z.lower : v->z;
        frustum->world_bounds.x.upper = frustum->world_bounds.x.upper <= v->x ? v->x : frustum->world_bounds.x.upper;
        frustum->world_bounds.y.upper = frustum->world_bounds.y.upper <= v->y ? v->y : frustum->world_bounds.y.upper;
        frustum->world_bounds.z.upper = frustum->world_bounds.z.upper <= v->z ? v->z : frustum->world_bounds.z.upper;
    }

    if (!build_projection) {
        frustum->projection_valid = 0;
        return;
    }

    {
        real_plane3d clip;
        real inverse_k;
        real e;
        real q;
        real qi;
        real qj;
        real qd;
        int16_t row;
        int16_t column;

        if (camera->z_near == 0.0f) {
            halo::math::matrix4x3_transform_plane(clip, frustum->world_to_view, camera->mirror_plane);
        } else {
            clip.normal.i = 0.0f;
            clip.normal.j = 0.0f;
            clip.normal.k = 1.0f;
            clip.d = -camera->z_near;
        }
        inverse_k = 1.0f / clip.normal.k;
        e = -(clip.d * inverse_k);
        q = camera->z_far / ((camera->z_far - e) *
                             ((real)fabs(clip.normal.j * inverse_k) +
                              (real)fabs(clip.normal.i * inverse_k) + 1.0f));
        qi = q * clip.normal.i * inverse_k;
        qj = q * clip.normal.j * inverse_k;
        qd = -(q * e);
        if (qd > 0.0f && camera->z_near == 0.0f) {
            qi = -qi;
            qj = -qj;
            q = -q;
            qd = -qd;
        }

        for (row = 0; row < 4; row++) {
            for (column = 0; column < 4; column++) {
                frustum->projection[row][column] = 0.0f;
            }
        }
        frustum->projection[0][2] = -qi;
        frustum->projection[1][2] = -qj;
        frustum->projection[2][0] = -cx;
        frustum->projection[0][0] = sx;
        frustum->projection[2][1] = -cy;
        frustum->projection[1][1] = sy;
        frustum->projection[2][3] = -1.0f;
        frustum->projection_valid = 1;
        frustum->projection[2][2] = -q;
        frustum->projection[3][2] = qd;
        frustum->projection_world_to_screen.i = sx * width * 0.5f;
        frustum->projection_world_to_screen.j = sy * height * 0.5f;
    }
}

/**
 * Scales the camera's asymmetric projection skew terms (bounds_in) by the field of view and
 * viewport aspect ratio to produce near-plane frustum extents (bounds_out), validating both the
 * input ordering (bounds_in[0] < bounds_in[1] and bounds_in[2] < bounds_in[3]) and the result
 * before accepting it. Falls back to the symmetric unit bounds (-1, 1, -1, 1) and returns 0 when
 * the asymmetric computation is disabled or its input or output is out of order.
 *
 * @address 0x0050cb70
 */
uint32_t compute_frustum_bounds(render_camera *camera, float bounds_out[4], float bounds_in[4])
{
    if (!render_asymmetric_frustum_disabled && bounds_in[0] < bounds_in[1] && bounds_in[2] < bounds_in[3]) {
        float inverse_tan_half_fov = 1.0f / (float)ftan((double)(camera->vertical_field_of_view * 0.5f));
        float aspect_scale = ((float)(camera->viewport_bounds.bottom - camera->viewport_bounds.top) /
                               (float)(camera->viewport_bounds.right - camera->viewport_bounds.left)) *
                              inverse_tan_half_fov;

        bounds_out[0] = aspect_scale * bounds_in[0];
        bounds_out[1] = aspect_scale * bounds_in[1];
        bounds_out[2] = inverse_tan_half_fov * bounds_in[2];
        bounds_out[3] = inverse_tan_half_fov * bounds_in[3];

        if (bounds_out[0] < bounds_out[1] && bounds_out[2] < bounds_out[3]) {
            return 1;
        }
    }

    bounds_out[3] = 1.0f;
    bounds_out[1] = 1.0f;
    bounds_out[2] = -1.0f;
    bounds_out[0] = -1.0f;
    return 0;
}

/**
 * Computes the four asymmetric-projection skew terms for a camera whose viewport is a sub-
 * rectangle of its window (e.g. a split-screen pane): bounds_out[0..1] are the left/right terms
 * (scaled by the viewport's aspect ratio against its own width), and bounds_out[2..3] are the
 * (negated) bottom/top terms, all normalized by the window's height.
 *
 * @address 0x0050ca90
 */
void compute_projection_skew(render_camera *camera, float bounds_out[4])
{
    float aspect = (float)(camera->viewport_bounds.bottom - camera->viewport_bounds.top) /
                   (float)(camera->viewport_bounds.right - camera->viewport_bounds.left);
    float inverse_window_height = 1.0f / (float)(camera->window_bounds.bottom - camera->window_bounds.top);
    float left = (float)(camera->viewport_bounds.left * 2 - camera->window_bounds.left -
                          camera->window_bounds.right) * inverse_window_height;
    float right = (float)(camera->viewport_bounds.right * 2 - camera->window_bounds.left -
                           camera->window_bounds.right) * inverse_window_height;
    float term_from_viewport_top = (float)(camera->viewport_bounds.top * 2 - camera->window_bounds.bottom -
                                            camera->window_bounds.top) * inverse_window_height;
    float term_from_viewport_bottom = (float)(camera->viewport_bounds.bottom * 2 - camera->window_bounds.bottom -
                                               camera->window_bounds.top) * inverse_window_height;

    bounds_out[0] = aspect * left;
    bounds_out[1] = aspect * right;
    bounds_out[2] = -term_from_viewport_bottom;
    bounds_out[3] = -term_from_viewport_top;
}

/**
 * Builds a camera-facing frame from the render globals: a direction vector plus a plane distance placed at the given distance from the camera.
 *
 * @address 0x00458990
 */
void facing_frame_build(float *out, float distance)
{
    float px = camera_forward_x[0] * distance + render_camera_global[0];
    float py = camera_forward_x[1] * distance + render_camera_global[1];
    float pz = camera_forward_x[2] * distance + render_camera_global[2];
    int32_t i;

    out[0x10] = camera_forward_x[0];
    out[0x11] = camera_forward_x[1];
    out[0x12] = camera_forward_x[2];
    out[0x13] = px * out[0x10] + py * out[0x11] + pz * out[0x12];

    for (i = 0; i < 16; i++) {
        out[i] = render_camera_facing_basis[i];
    }
}

/**
 * EAX -> a, ECX -> b
 * Builds a mirrored copy of source_camera across the mirror's plane: for an ordinary mirror
 * (mirror->shader_mirror_value_0 == 0) this is a full reflection of position, forward and up (with a small
 * correction when the camera looks nearly edge-on into the plane, and an extra flip of the
 * reflected up vector to keep the basis right-handed for the rasterizer); for a portal-style
 * mirror it instead shifts the camera along the plane's normal by an amount driven by the mirror
 * shader's two runtime floats. Either way the mirror flag is set (or cleared) and the camera's
 * near clip plane and mirror_plane fields are stamped for the rasterizer.
 *
 * @address 0x0050c660
 */
void mirror(render_camera *source_camera, structure_bsp_mirror_result *mirror, render_camera *out_camera)
{
    real_vector3d plane_normal = mirror->plane.normal;
    float plane_d = mirror->plane.d;

    *out_camera = *source_camera;

    if (mirror->shader_mirror_value_0 == 0.0f) {
        real_vector3d reflect_dir = plane_normal;
        float baseline = plane_d;
        float dot_forward;
        float dot_up;
        float dot_position;

        if ((float)fabs((double)(plane_normal.i * source_camera->forward.i +
                                  plane_normal.j * source_camera->forward.j +
                                  plane_normal.k * source_camera->forward.k)) < 0.0125f) {
            float signed_offset = -((plane_normal.j * source_camera->position.y +
                                      plane_normal.k * source_camera->position.z +
                                      plane_normal.i * source_camera->position.x) - plane_d);
            reflect_dir.i = source_camera->forward.i * 0.005859375f + plane_normal.i;
            reflect_dir.j = source_camera->forward.j * 0.005859375f + plane_normal.j;
            reflect_dir.k = source_camera->forward.k * 0.005859375f + plane_normal.k;
            halo::math::vector3d_normalize_with_length(reflect_dir);
            baseline = reflect_dir.k * (plane_normal.k * signed_offset + source_camera->position.z) +
                       reflect_dir.j * (plane_normal.j * signed_offset + source_camera->position.y) +
                       reflect_dir.i * (plane_normal.i * signed_offset + source_camera->position.x);
        }

        dot_forward = (reflect_dir.i * source_camera->forward.i + reflect_dir.j * source_camera->forward.j +
                       reflect_dir.k * source_camera->forward.k) * 2.0f;
        out_camera->forward.i = source_camera->forward.i - reflect_dir.i * dot_forward;
        out_camera->forward.j = source_camera->forward.j - reflect_dir.j * dot_forward;
        out_camera->forward.k = source_camera->forward.k - reflect_dir.k * dot_forward;

        dot_up = (reflect_dir.i * source_camera->up.i + reflect_dir.j * source_camera->up.j +
                  reflect_dir.k * source_camera->up.k) * 2.0f;
        out_camera->up.i = source_camera->up.i - reflect_dir.i * dot_up;
        out_camera->up.j = source_camera->up.j - reflect_dir.j * dot_up;
        out_camera->up.k = source_camera->up.k - reflect_dir.k * dot_up;

        dot_position = ((reflect_dir.j * source_camera->position.y + reflect_dir.k * source_camera->position.z +
                         reflect_dir.i * source_camera->position.x) - baseline) * -2.0f;
        out_camera->position.x = reflect_dir.i * dot_position + source_camera->position.x;
        out_camera->position.y = reflect_dir.j * dot_position + source_camera->position.y;
        out_camera->position.z = reflect_dir.k * dot_position + source_camera->position.z;

        out_camera->mirrored = (uint8_t)(source_camera->mirrored == 0);

        out_camera->up.i = -out_camera->up.i;
        out_camera->up.j = -out_camera->up.j;
        out_camera->up.k = -out_camera->up.k;
    } else {
        float inverse_forward_length =
            1.0f / (float)sqrt((double)(source_camera->forward.k * source_camera->forward.k +
                                         source_camera->forward.j * source_camera->forward.j +
                                         source_camera->forward.i * source_camera->forward.i));
        float sin_angle = halo::math::vector3d_cross_product_length(plane_normal, source_camera->forward) *
                           inverse_forward_length;
        float shift = sin_angle * mirror->shader_mirror_value_0;

        if (sin_angle == 0.0f) {
            shift = 0.0f;
        } else {
            float dot_normal_forward = plane_normal.i * source_camera->forward.i +
                                        plane_normal.j * source_camera->forward.j +
                                        plane_normal.k * source_camera->forward.k;
            shift = -((dot_normal_forward * inverse_forward_length * shift * mirror->shader_mirror_value_1) /
                      ((float)sqrt((double)(1.0f - shift * shift)) * sin_angle));
        }

        out_camera->position.x = plane_normal.i * shift + source_camera->position.x;
        out_camera->position.y = plane_normal.j * shift + source_camera->position.y;
        out_camera->position.z = plane_normal.k * shift + source_camera->position.z;
    }

    out_camera->z_near = 0.0f;
    out_camera->mirror_plane.normal = plane_normal;
    out_camera->mirror_plane.d = plane_d;
}

/**
 * Three-way depth-range helper for the projection matrix's z column (projection[0..3][2]):
 * z_near == z_far == -1.0 saves the frustum's current z-column terms to render_saved_projection_z
 * ("push"); z_near == z_far == 0.0 restores them ("pop"); any other pair recomputes the z-column
 * terms for that near/far range (the standard perspective z-range terms, with this engine's own
 * sign convention), used to temporarily bias the depth range (e.g. for overlays/decals) without
 * touching the rest of the projection matrix.
 *
 * @address 0x0050c9a0
 */
void projection_zrange_push_pop_set(render_frustum *frustum, float z_near, float z_far)
{
    if (z_near == -1.0f && z_far == -1.0f) {
        render_saved_projection_z[0] = frustum->projection[0][2];
        render_saved_projection_z[1] = frustum->projection[1][2];
        render_saved_projection_z[2] = frustum->projection[2][2];
        render_saved_projection_z[3] = frustum->projection[3][2];
        return;
    }
    if (z_near == 0.0f && z_far == 0.0f) {
        frustum->projection[0][2] = render_saved_projection_z[0];
        frustum->projection[1][2] = render_saved_projection_z[1];
        frustum->projection[2][2] = render_saved_projection_z[2];
        frustum->projection[3][2] = render_saved_projection_z[3];
        return;
    }
    frustum->projection[0][2] = 0.0f;
    frustum->projection[1][2] = 0.0f;
    frustum->projection[2][2] = -((z_near + z_far) / (z_far - z_near));
    frustum->projection[3][2] = (z_near * z_far * -2.0f) / (z_far - z_near);
}

}  // namespace halo::render::camera

namespace halo::render::frustum {

/**
 * Classifies a world-space point against the camera frustum's four side planes (not near/far),
 * returning a 4-bit outcode: one bit per plane the point is in front of (outside).
 *
 * @address 0x0050d4c0
 */
uint8_t classify_point_side_planes(render_frustum *frustum, real_point3d *point)
{
    uint8_t flags = 0;

    if ((frustum->world_planes[0].normal.i * point->x + frustum->world_planes[0].normal.k * point->z +
         frustum->world_planes[0].normal.j * point->y) - frustum->world_planes[0].d > 0.0f) {
        flags |= _render_frustum_point_plane0_bit;
    }
    if ((frustum->world_planes[1].normal.i * point->x + frustum->world_planes[1].normal.k * point->z +
         frustum->world_planes[1].normal.j * point->y) - frustum->world_planes[1].d > 0.0f) {
        flags |= _render_frustum_point_plane1_bit;
    }
    if ((frustum->world_planes[2].normal.i * point->x + frustum->world_planes[2].normal.k * point->z +
         frustum->world_planes[2].normal.j * point->y) - frustum->world_planes[2].d > 0.0f) {
        flags |= _render_frustum_point_plane2_bit;
    }
    if ((frustum->world_planes[3].normal.i * point->x + frustum->world_planes[3].normal.k * point->z +
         frustum->world_planes[3].normal.j * point->y) - frustum->world_planes[3].d > 0.0f) {
        flags |= _render_frustum_point_plane3_bit;
    }

    return flags;
}

/**
 * Projects a view-space axis-aligned box (as seen at both its nearest and farthest z) onto the
 * screen and returns the area of its clamped (-1..1) NDC bounding rectangle as a fraction of the
 * full 2x2 NDC area (0 when the box's projected x or y range does not overlap the screen at all).
 * If the box straddles or is behind the camera (z.lower < 0 <= z.upper) it is treated as fully
 * covering the view and this returns 1.0.
 *
 * @address 0x0050dac0
 */
real compute_box_overlap_area(real_rectangle3d *box, render_frustum *frustum)
{
    float inverse_z_near, inverse_z_far;
    float candidate_a, candidate_b;
    float x_lo, x_hi, y_lo, y_hi;
    float area;

    if (box->z.lower >= 0.0f) {
        return 0.0f;
    }
    if (box->z.upper >= 0.0f) {
        return 1.0f;
    }

    inverse_z_near = -1.0f / box->z.lower;
    inverse_z_far = -1.0f / box->z.upper;

    candidate_a = (frustum->projection[2][0] * box->z.lower + frustum->projection[0][0] * box->x.lower) * inverse_z_near;
    candidate_b = (frustum->projection[2][0] * box->z.upper + frustum->projection[0][0] * box->x.lower) * inverse_z_far;
    x_lo = (candidate_a < candidate_b) ? candidate_a : candidate_b;
    if (x_lo < -1.0f) {
        x_lo = -1.0f;
    }

    candidate_a = (frustum->projection[2][1] * box->z.lower + frustum->projection[1][1] * box->y.lower) * inverse_z_near;
    candidate_b = (frustum->projection[2][1] * box->z.upper + frustum->projection[1][1] * box->y.lower) * inverse_z_far;
    y_lo = (candidate_a < candidate_b) ? candidate_a : candidate_b;
    if (y_lo < -1.0f) {
        y_lo = -1.0f;
    }

    candidate_a = (frustum->projection[2][0] * box->z.lower + frustum->projection[0][0] * box->x.upper) * inverse_z_near;
    candidate_b = (frustum->projection[2][0] * box->z.upper + frustum->projection[0][0] * box->x.upper) * inverse_z_far;
    x_hi = (candidate_a > candidate_b) ? candidate_a : candidate_b;
    if (x_hi > 1.0f) {
        x_hi = 1.0f;
    }

    candidate_a = (frustum->projection[2][1] * box->z.lower + frustum->projection[1][1] * box->y.upper) * inverse_z_near;
    candidate_b = (frustum->projection[2][1] * box->z.upper + frustum->projection[1][1] * box->y.upper) * inverse_z_far;
    y_hi = (candidate_a > candidate_b) ? candidate_a : candidate_b;
    if (y_hi > 1.0f) {
        y_hi = 1.0f;
    }

    area = (x_hi - x_lo) * (y_hi - y_lo) * 0.25f;
    return (area > 0.0f) ? area : 0.0f;
}

/**
 * Computes the normalized (-1..1) screen-space clip rectangle (left, right, bottom, top) implied
 * by the frustum's current projection matrix, from its two diagonal scale terms and its
 * (asymmetric) perspective shear terms.
 *
 * @address 0x0050ddc0
 */
void compute_screen_clip_bounds(float out[4], render_frustum *frustum)
{
    float shear_x = frustum->projection[2][0];
    float shear_y = frustum->projection[2][1];
    float inverse_scale_x = -1.0f / frustum->projection[0][0];
    float inverse_scale_y = -1.0f / frustum->projection[1][1];

    out[0] = (-shear_x - 1.0f) * inverse_scale_x;
    out[1] = (-shear_x + 1.0f) * inverse_scale_x;
    out[2] = (-shear_y - 1.0f) * inverse_scale_y;
    out[3] = (-shear_y + 1.0f) * inverse_scale_y;
}

/**
 * module
 * Classifies an axis-aligned world-space bounding box against the camera frustum: fully outside,
 * fully inside, or intersecting. First rejects boxes that do not even overlap the frustum's own
 * overall AABB (world_bounds), then classifies all 8 box corners against the four side planes
 * (via render_frustum_classify_point_side_planes) to decide outside/inside/intersecting. When
 * validate is set and the side-plane test says "intersecting", a further separating-axis check
 * against the frustum's own extreme vertices (world_vertices) can still reject the box.
 *
 * @address 0x0050d5b0
 */
int16_t test_bounding_box(render_frustum *frustum, real_rectangle3d *box, uint8_t validate)
{
    real_point3d corners[8];
    uint8_t all_outside = 0x3f;
    uint8_t any_outside = 0;
    int i;

    if (frustum->world_bounds.x.upper < box->x.lower || frustum->world_bounds.y.upper < box->y.lower ||
        frustum->world_bounds.z.upper < box->z.lower || box->x.upper < frustum->world_bounds.x.lower ||
        box->y.upper < frustum->world_bounds.y.lower || box->z.upper < frustum->world_bounds.z.lower) {
        return _render_frustum_outside;
    }

    corners[0].x = box->x.lower; corners[0].y = box->y.lower; corners[0].z = box->z.lower;
    corners[1].x = box->x.upper; corners[1].y = box->y.lower; corners[1].z = box->z.lower;
    corners[2].x = box->x.lower; corners[2].y = box->y.upper; corners[2].z = box->z.lower;
    corners[3].x = box->x.upper; corners[3].y = box->y.upper; corners[3].z = box->z.lower;
    corners[4].x = box->x.lower; corners[4].y = box->y.lower; corners[4].z = box->z.upper;
    corners[5].x = box->x.upper; corners[5].y = box->y.lower; corners[5].z = box->z.upper;
    corners[6].x = box->x.lower; corners[6].y = box->y.upper; corners[6].z = box->z.upper;
    corners[7].x = box->x.upper; corners[7].y = box->y.upper; corners[7].z = box->z.upper;

    for (i = 0; i < 8; i++) {
        uint8_t flags = halo::render::render_frustum_classify_point_side_planes(frustum, &corners[i]);
        all_outside &= flags;
        any_outside |= flags;
    }

    if (any_outside == 0) {
        return _render_frustum_inside;
    }
    if (all_outside != 0) {
        return _render_frustum_outside;
    }

    if (validate) {
        uint8_t vertex_separates = 0x3f;

        for (i = 0; i < 5; i++) {
            real_point3d *v = &frustum->world_vertices[i];
            uint8_t bits = 0;

            if (v->x <= box->x.lower) bits |= 0x01;
            if (v->x >= box->x.upper) bits |= 0x02;
            if (v->y >= box->y.upper) bits |= 0x04;
            if (v->y <= box->y.lower) bits |= 0x08;
            if (v->z <= box->z.lower) bits |= 0x10;
            if (v->z >= box->z.upper) bits |= 0x20;

            vertex_separates &= bits;
        }

        if (vertex_separates != 0) {
            return _render_frustum_outside;
        }
    }

    return _render_frustum_partial;
}

/**
 * Classifies a world-space sphere against the camera frustum: outside, partially inside (straddles
 * at least one plane) or fully inside. First rejects spheres that do not even overlap the
 * frustum's own overall AABB (world_bounds) or that are entirely outside any one of the six
 * frustum planes by more than the radius; a sphere that passes is then fully inside only if it
 * also sits more than a radius on the inside of five of those six planes (see the UNSURE note
 * above for the sixth).
 *
 * @address 0x0050d890
 */
int16_t test_sphere(render_frustum *frustum, real_point3d *center, float radius)
{
    float distance0, distance1, distance2, distance3, distance5;
    float negative_radius;

    if (center->x - radius > frustum->world_bounds.x.upper ||
        center->y - radius > frustum->world_bounds.y.upper ||
        center->z - radius > frustum->world_bounds.z.upper ||
        frustum->world_bounds.x.lower > radius + center->x ||
        frustum->world_bounds.y.lower > radius + center->y ||
        frustum->world_bounds.z.lower > radius + center->z) {
        return _render_frustum_outside;
    }

    distance0 = (frustum->world_planes[0].normal.i * center->x + frustum->world_planes[0].normal.j * center->y +
                 frustum->world_planes[0].normal.k * center->z) - frustum->world_planes[0].d;
    if (distance0 > radius) {
        return _render_frustum_outside;
    }
    distance1 = (frustum->world_planes[1].normal.i * center->x + frustum->world_planes[1].normal.j * center->y +
                 frustum->world_planes[1].normal.k * center->z) - frustum->world_planes[1].d;
    if (distance1 > radius) {
        return _render_frustum_outside;
    }
    distance2 = (frustum->world_planes[2].normal.i * center->x + frustum->world_planes[2].normal.j * center->y +
                 frustum->world_planes[2].normal.k * center->z) - frustum->world_planes[2].d;
    if (distance2 > radius) {
        return _render_frustum_outside;
    }
    distance3 = (frustum->world_planes[3].normal.i * center->x + frustum->world_planes[3].normal.j * center->y +
                 frustum->world_planes[3].normal.k * center->z) - frustum->world_planes[3].d;
    if (distance3 > radius) {
        return _render_frustum_outside;
    }
    if ((frustum->world_planes[4].normal.i * center->x + frustum->world_planes[4].normal.j * center->y +
         frustum->world_planes[4].normal.k * center->z) - frustum->world_planes[4].d > radius) {
        return _render_frustum_outside;
    }
    distance5 = (frustum->world_planes[5].normal.i * center->x + frustum->world_planes[5].normal.j * center->y +
                 frustum->world_planes[5].normal.k * center->z) - frustum->world_planes[5].d;
    if (distance5 > radius) {
        return _render_frustum_outside;
    }

    negative_radius = -radius;
    if (negative_radius <= distance0 || negative_radius <= distance1 || negative_radius <= distance2 ||
        negative_radius <= distance3 || negative_radius <= distance5) {
        return _render_frustum_partial;
    }
    return _render_frustum_inside;
}

}  // namespace halo::render::frustum

namespace halo::render::frame {

/**
 * Projects a world-space point into the camera's normalized device coordinates and then into
 * screen pixel coordinates, returning whether it lies within the view (in front of the camera,
 * i.e. negative view-space z, and inside the -1..1 clip range on both axes).
 *
 * @address 0x0050de30
 */
uint8_t project_world_point_to_screen(real_point2d *screen_out, real_point3d *world_point, render_frustum *frustum,
    render_camera *camera)
{
    float inverse_z;
    float clip_x, clip_y;

    if (world_point->z >= 0.0f) {
        return 0;
    }

    inverse_z = -1.0f / world_point->z;
    clip_x = (frustum->projection[2][0] * world_point->z + frustum->projection[0][0] * world_point->x) * inverse_z;
    clip_y = -((frustum->projection[2][1] * world_point->z + frustum->projection[1][1] * world_point->y) * inverse_z);

    screen_out->x = clip_x;
    screen_out->y = clip_y;
    if (clip_x < -1.0f || clip_x > 1.0f || clip_y < -1.0f || clip_y > 1.0f) {
        return 0;
    }

    screen_out->x = (screen_out->x + 1.0f) * 0.5f * (float)k_render_virtual_screen_width +
                    (float)camera->viewport_bounds.left;
    screen_out->y = (screen_out->y + 1.0f) * 0.5f * (float)k_render_virtual_screen_height +
                    (float)camera->viewport_bounds.top;
    return 1;
}

}  // namespace halo::render::frame
