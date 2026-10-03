#include "halo/game/gamerest_math.hpp"
#include <stdint.h>
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/units/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"

extern "C" {
extern double sqrt(double x);
}
static auto &response_curve_scale_limit = halo::link::ref<double>(halo::game::vars().response_curve_scale_limit);
extern "C" {
extern double fabs(double x);
}
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &team_pair_data = halo::link::ref<team_pair_globals *>(halo::ai::vars().team_pair_data);
static auto &main_game_globals = halo::link::ref<void *>(halo::game::vars().main_game_globals);
static auto &weapon_zoom_index_substitutions = halo::link::ref<int16_t []>(halo::game::vars().weapon_zoom_index_substitutions);
extern "C" {
extern int32_t __ftol(void);
}

namespace halo::game {

/**
 * Implements the original `angle_delta_wrapped`.
 *
 * @address 0x470d10
 */
float ScalarMath::angle_delta_wrapped(float from, float to)
{
    float delta = to - from;

    if (3.1415927f <= delta) {
        delta = delta - 6.2831855f;
    }
    if (delta <= -3.1415927f) {
        delta = delta + 6.2831855f;
    }
    return delta;
}

/**
 * Implements the original `control_axis_sign`.
 *
 * @address 0x471070
 */
real ScalarMath::control_axis_sign(real value)
{
    real result = 0.0f;

    if (value > 0.05f) {
        result = 1.0f;
    }
    if (value < -0.05f) {
        result = -1.0f;
    }
    return result;
}

/**
 * Implements the original `distance_falloff_fraction`.
 *
 * @address 0x459360
 */
real ScalarMath::distance_falloff_fraction(real value, real max_range)
{
    real half_range;

    half_range = max_range * 0.5f;
    if (value >= max_range) {
        return 0.0f;
    }
    if (value <= half_range) {
        return 1.0f;
    }
    return (max_range - value) / (max_range - half_range);
}

/**
 * Projects `reference_point` onto the target unit's look ray (its eye origin extended along its
 * looking direction), clamping the projection to the ray's forward half, then nudges the result
 * back along `aux_vector` by the (clamp-limited) component of the raw delta along that axis.
 * REWRITTEN 2026-09-27 (static loop) from objdump 0x45a280..0x45a494: only TWO stack arguments (reference_point,
 * out_closest). The nudge clamp is the float unit_get_look_origin_and_direction writes through its second argument
 * (the target tag's autoaim width, +0x458) -- the binary passes the address of its own out_closest slot, having saved
 * out_closest in EBP. The draft took an extra clamp parameter, which the callers filled with 0.0 / a cone distance.
 *
 * @address 0x45a280
 */
void ScalarMath::closest_point_on_segment(datum_index unit_index, real_vector3d *aux_vector, real_point3d *reference_point, real_point3d *out_closest)
{
    real_vector3d direction;
    real_point3d origin;
    uint32_t autoaim_width_bits;
    real_vector3d cross;
    real cross_length_squared;
    real t;
    real_vector3d delta;
    real_vector3d nudge;
    real dot;

    halo::units::unit_get_look_origin_and_direction(unit_index, &autoaim_width_bits, &direction, &origin);

    cross.i = direction.j * aux_vector->k - direction.k * aux_vector->j;
    cross.j = direction.k * aux_vector->i - direction.i * aux_vector->k;
    cross.k = direction.i * aux_vector->j - direction.j * aux_vector->i;
    cross_length_squared = cross.i * cross.i + cross.j * cross.j + cross.k * cross.k;

    if (cross_length_squared <= 0.0f) {
        *out_closest = origin;
    } else {
        t = (((reference_point->y - origin.y) * aux_vector->k - (reference_point->z - origin.z) * aux_vector->j) * cross.i +
             ((reference_point->z - origin.z) * aux_vector->i - (reference_point->x - origin.x) * aux_vector->k) * cross.j +
             ((reference_point->x - origin.x) * aux_vector->j - (reference_point->y - origin.y) * aux_vector->i) * cross.k) /
            cross_length_squared;
        if (t < 0.0f) {
            t = 0.0f;
        } else if (1.0f < t) {
            t = 1.0f;
        }
        out_closest->x = direction.i * t + origin.x;
        out_closest->y = direction.j * t + origin.y;
        out_closest->z = direction.k * t + origin.z;
    }

    delta.i = out_closest->x - reference_point->x;
    delta.j = out_closest->y - reference_point->y;
    delta.k = out_closest->z - reference_point->z;
    dot = -(delta.j * aux_vector->j + delta.k * aux_vector->k + delta.i * aux_vector->i);

    nudge.i = dot * aux_vector->i + delta.i;
    nudge.j = dot * aux_vector->j + delta.j;
    nudge.k = dot * aux_vector->k + delta.k;
    Vector3dView(&nudge).clamp_length(*(real *)&autoaim_width_bits);

    out_closest->x = out_closest->x - nudge.i;
    out_closest->y = out_closest->y - nudge.j;
    out_closest->z = out_closest->z - nudge.k;
}

/**
 * Scales `v` down so its length does not exceed `max_length`; leaves `v` unchanged when it is
 * already within bounds.
 *
 * @address 0x459300
 */
void Vector3dView::clamp_length(real max_length)
{
    real length_squared;
    real max_squared;
    real scale;

    length_squared = v->k * v->k + v->j * v->j + v->i * v->i;
    max_squared = max_length * max_length;
    if (length_squared >= max_squared && length_squared != max_squared) {
        scale = max_length / (real)sqrt((double)length_squared);
        v->i = scale * v->i;
        v->j = scale * v->j;
        v->k = scale * v->k;
    }
}

/**
 * Implements the original `value_step_toward_target`.
 *
 * @address 0x470d40
 */
void FloatValue::step_toward_target(float target, float max_step)
{
    float delta = target - *value;

    if (delta < -max_step) {
        delta = -max_step;
    } else if (delta > max_step) {
        delta = max_step;
    }
    *value = *value + delta;
}

/**
 * Implements the original `point3d_array_project_to_xy_plane`.
 *
 * @address 0x46a130
 */
void Point3dArray::project_to_xy_plane(Point2D *destination, int32_t count)
{
    int32_t i;
    for (i = 0; i < count; i++) {
        destination[i].x = source[i].x;
        destination[i].y = source[i].y;
    }
}

/**
 * blam-cc: EDI -> table, stack -> table_count, x
 * Evaluates the `table_count`-entry piecewise-linear table at fractional position
 * |x| * (table_count - 1) (clamped to at most table_count - 1 and to response_curve_scale_limit),
 * linearly interpolating between the two nearest entries, and negates the result if x < 0.
 *
 * @address 0x470fb0
 */
real ResponseCurve::evaluate(int16_t table_count, real x, real *table)
{
    int32_t max_index = table_count - 1;
    double scaled = fabs((double)x) * (double)max_index;
    int32_t lower, upper;
    real result;

    if (scaled < response_curve_scale_limit) {
        scaled = response_curve_scale_limit;
    } else if (scaled > (double)max_index) {
        scaled = (double)max_index;
    }

    lower = (int32_t)scaled;
    if (lower < 0) {
        lower = 0;
    } else if (lower > max_index) {
        lower = max_index;
    }
    upper = lower + 1;
    if (upper > max_index) {
        upper = max_index;
    }

    result = (real)(scaled - (double)lower) * (table[upper] - table[lower]) + table[lower];
    if (x < 0.0f) {
        result = -result;
    }
    return result;
}

/**
 * blam-cc: CX -> magnification, stack -> zoom_table_index
 * Returns the field-of-view multiplier for zoom level `magnification` (clamped to 0..3, or, for
 * a negative magnification, level 0) of the zoom table at row `zoom_table_index`, or 1.0 if the
 * globals tag or its zoom table is not loaded.
 *
 * @address 0x46fe10
 */
real WeaponZoom::get_zoom_fov(int16_t zoom_table_index, int16_t magnification)
{
    TagReflexive *zoom_table_reflexive;

    real *rows;

    if (global_globals == 0) {
        return 1.0f;
    }
    zoom_table_reflexive = (TagReflexive *)((uint8_t *)global_globals + 0x11c);
    if (zoom_table_reflexive->count == 0) {
        return 1.0f;
    }
    rows = (real *)(uintptr_t)zoom_table_reflexive->pointer;
    if (rows == 0) {
        return 1.0f;
    }

    if (magnification < 0) {
        return rows[(int32_t)zoom_table_index * 4];
    }
    if (magnification > 3) {
        magnification = 3;
    }
    return rows[(int32_t)zoom_table_index * 4 + magnification];
}

/**
 * blam-cc: ECX -> zoom_table_index, AX -> substitution_check_index
 * In multiplayer (current_game_engine != NULL), forces magnification to 1 and looks up
 * weapon_get_zoom_fov(zoom_table_index, 1) directly. Single-player: if
 * substitution_check_index is in 0..9 and team_pair_globals marks team `substitution_check_index`
 * as an enemy of team 1, first substitutes zoom_table_index through
 * weapon_zoom_index_substitutions (falling back to the multiplayer-style call if the substitute is
 * -1); either way, the magnification used is *(int16 *)(main_game_globals + 0xe).
 *
 * @address 0x46fe70
 */
real WeaponZoom::get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index)
{
    int16_t magnification = *(int16_t *)((uint8_t *)main_game_globals + 0xe);

    if (current_game_engine != 0) {
        return WeaponZoom::get_zoom_fov(zoom_table_index, 1);
    }

    if (substitution_check_index >= 0 && substitution_check_index < 10) {
        int32_t bit_index = substitution_check_index + 10;
        uint32_t bit = 1u << (bit_index & 0x1f);
        uint8_t is_enemy = (team_pair_data->enemy_bits[bit_index >> 5] & bit) != 0;

        if (is_enemy) {
            int16_t substitute = weapon_zoom_index_substitutions[(uint16_t)zoom_table_index];

            if (substitute == -1) {
                return WeaponZoom::get_zoom_fov(zoom_table_index, 1);
            }
            zoom_table_index = substitute;
        }
    }
    return WeaponZoom::get_zoom_fov(zoom_table_index, magnification);
}

/**
 * REWRITTEN from objdump 0x45f6e0..0x45f71c: EAX is the tag's first reflexive (count at +0, elements at +4, stride
 * 0x54). The running total starts at 0 and, for each element, becomes __ftol(total + element weight at +0x20) -- the
 * truncation happens after every add. Returns the total (0 for an empty block). The name predates this reading.
 * blam-cc: EAX -> reflexive
 *
 * @address 0x45f6e0
 */
int32_t RandomTable::advance_draws(TagReflexive *reflexive)
{
    int32_t count = (int32_t)reflexive->count;
    uint8_t *element = (uint8_t *)reflexive->pointer;
    int32_t total = 0;
    int32_t i;

    for (i = 0; i < count; i++) {
        total = (int32_t)((float)total + *(float *)(element + i * 0x54 + 0x20));
    }
    return total;
}

/**
 * blam-cc: EAX -> out
 * Advances the global LCG PRNG and writes a random entry of sphere_point_table into *out.
 *
 * @address 0x473560
 */
void RandomTable::get_table_point(real_point3d *out)
{
    int16_t index;

    halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
    index = (int16_t)(((halo::math::globals().random_seed_global >> 16) *
                       (uint32_t)(int32_t)(int16_t)halo::math::globals().sphere_point_table_count) >> 16);
    *out = halo::math::globals().sphere_point_table[index];
}

/**
 * REWRITTEN from objdump 0x45f720..0x45f7b0: total = the weight sum (16 bits used); a draw r = ((seed >> 16) * total)
 * >> 16 as int16; walking the elements, r = __ftol(r - weight) and the first element taking it below zero wins,
 * returning its dword at +0x30; -1 when none does.
 * blam-cc: EAX -> tag_id
 *
 * @address 0x45f720
 */
int32_t RandomTable::pick_weighted_random_index(datum_index tag_id)
{
    TagReflexive *reflexive = (TagReflexive *)halo::cache::globals().tag_instances[tag_id & 0xffff].data;
    int32_t count = (int32_t)reflexive->count;
    int16_t total = (int16_t)RandomTable::advance_draws(reflexive);
    uint8_t *element;
    int32_t remaining;
    int32_t i;

    halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
    remaining = (int16_t)(((halo::math::globals().random_seed_global >> 0x10) * (uint32_t)(int32_t)total) >> 0x10);
    element = (uint8_t *)reflexive->pointer;
    for (i = 0; i < count; i++) {
        remaining = (int32_t)((float)remaining - *(float *)(element + i * 0x54 + 0x20));
        if (remaining < 0) {
            return *(int32_t *)(element + i * 0x54 + 0x30);
        }
    }
    return -1;
}

/**
 * Implements the original `object_placement_data_set_change_colors`.
 *
 * @address 0x477670
 */
void PlacementData::set_change_colors(real *color)
{
    int32_t i;
    for (i = 0; i < 4; i = i + 1) {
        placement->network_vectors[i].i = color[0];
        placement->network_vectors[i].j = color[1];
        placement->network_vectors[i].k = color[2];
    }
}

}  // namespace halo::game

namespace halo::game {

/**
 * C entry point for halo::game::ScalarMath::angle_delta_wrapped; forwards to the C++ implementation.
 *
 * @address 0x470d10
 */
float angle_delta_wrapped(float from, float to)
{
    return halo::game::ScalarMath::angle_delta_wrapped(from, to);
}

/**
 * C entry point for halo::game::ScalarMath::control_axis_sign; forwards to the C++ implementation.
 *
 * @address 0x471070
 */
real control_axis_sign(real value)
{
    return halo::game::ScalarMath::control_axis_sign(value);
}

/**
 * C entry point for halo::game::ScalarMath::distance_falloff_fraction; forwards to the C++ implementation.
 * register convention: both operands are the recognized stack parameters (param_1, param_2);
 * no register arguments.
 * // blam-cc: stack -> value, max_range
 *
 * @address 0x459360
 */
real distance_falloff_fraction(real value, real max_range)
{
    return halo::game::ScalarMath::distance_falloff_fraction(value, max_range);
}

/**
 * C entry point for halo::game::ScalarMath::closest_point_on_segment; forwards to the C++ implementation.
 * register convention: target unit index in ECX (in_ECX, forwarded to unit_get_look_origin_
 * and_direction), a third vector in EBX (unaff_EBX) whose real identity was not pinned down
 * (see UNSURE below), reference point and output point as the two recognized stack parameters
 * (param_1, param_2).
 * // blam-cc: ECX -> unit_index, EBX -> aux_vector, stack -> reference_point, out_closest
 * blam-cc: ECX -> unit_index, EBX -> aux_vector, stack -> reference_point, out_closest
 *
 * @address 0x45a280
 */
void vector3d_closest_point_on_segment(datum_index unit_index, real_vector3d *aux_vector, real_point3d *reference_point, real_point3d *out_closest)
{
    halo::game::ScalarMath::closest_point_on_segment(unit_index, aux_vector, reference_point, out_closest);
}

/**
 * C entry point for halo::game::Vector3dView::clamp_length; forwards to the C++ implementation.
 * register convention: vector pointer in ECX (in_ECX), maximum length as the recognized stack
 * parameter (param_1).
 * // blam-cc: ECX -> v, stack -> max_length
 *
 * @address 0x459300
 */
void vector3d_clamp_length(real_vector3d *v, real max_length)
{
    halo::game::Vector3dView(v).clamp_length(max_length);
}

/**
 * C entry point for halo::game::FloatValue::step_toward_target; forwards to the C++ implementation.
 * register convention: the value pointer is Ghidra's `in_ECX`; `target` and `max_step` are this
 * blam-cc: ECX -> value, stack -> target, max_step
 *
 * @address 0x470d40
 */
void value_step_toward_target(float *value, float target, float max_step)
{
    halo::game::FloatValue(value).step_toward_target(target, max_step);
}

/**
 * C entry point for halo::game::Point3dArray::project_to_xy_plane; forwards to the C++ implementation.
 * register convention: source real_point3d array in the stack parameter (Ghidra's own
 * `param_1`); destination Point2D array in unaff_EDI; element count in unaff_EBX.
 * // blam-cc: stack -> source, EDI -> destination, EBX -> count
 *
 * @address 0x46a130
 */
void point3d_array_project_to_xy_plane(real_point3d *source, Point2D *destination, int32_t count)
{
    halo::game::Point3dArray(source).project_to_xy_plane(destination, count);
}

/**
 * C entry point for halo::game::ResponseCurve::evaluate; forwards to the C++ implementation.
 * register convention: the float table pointer is Ghidra's `unaff_EDI`.
 * // blam-cc: EDI -> table, stack -> table_count, x
 * blam-cc: EDI -> table, stack -> table_count, x
 *
 * @address 0x470fb0
 */
real response_curve_evaluate(int16_t table_count, real x, real *table)
{
    return halo::game::ResponseCurve::evaluate(table_count, x, table);
}

/**
 * C entry point for halo::game::WeaponZoom::get_zoom_fov; forwards to the C++ implementation.
 * register convention: objdump 0x46fe10 shows no prologue push/sub, so `zoom_table_index` is a
 * blam-cc: CX -> magnification, stack -> zoom_table_index
 * blam-cc: CX -> magnification, stack -> zoom_table_index
 *
 * @address 0x46fe10
 */
real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification)
{
    return halo::game::WeaponZoom::get_zoom_fov(zoom_table_index, magnification);
}

/**
 * C entry point for halo::game::WeaponZoom::get_zoom_fov_resolved; forwards to the C++ implementation.
 * register convention: ECX carries the zoom-table index (`mov edi,ecx` is the first instruction);
 * blam-cc: ECX -> zoom_table_index, AX -> substitution_check_index
 * blam-cc: ECX -> zoom_table_index, AX -> substitution_check_index
 *
 * @address 0x46fe70
 */
real weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index)
{
    return halo::game::WeaponZoom::get_zoom_fov_resolved(zoom_table_index, substitution_check_index);
}

/**
 * C entry point for halo::game::RandomTable::advance_draws; forwards to the C++ implementation.
 * register convention: int *count in EAX (in_EAX).
 * // blam-cc: EAX -> count
 * blam-cc: EAX -> reflexive
 *
 * @address 0x45f6e0
 */


/**
 * C entry point for halo::game::RandomTable::get_table_point; forwards to the C++ implementation.
 * register convention: output point pointer in EAX (Ghidra's `in_EAX`).
 * // blam-cc: EAX -> out
 * blam-cc: EAX -> out
 *
 * @address 0x473560
 */
void random_get_table_point(real_point3d *out)
{
    halo::game::RandomTable::get_table_point(out);
}

/**
 * C entry point for halo::game::RandomTable::pick_weighted_random_index; forwards to the C++ implementation.
 * register convention: tag id in EAX (in_EAX).
 * // blam-cc: EAX -> tag_id
 * blam-cc: EAX -> tag_id
 *
 * @address 0x45f720
 */
int32_t tag_reflexive_pick_weighted_random_index(datum_index tag_id)
{
    return halo::game::RandomTable::pick_weighted_random_index(tag_id);
}

/**
 * C entry point for halo::game::PlacementData::set_change_colors; forwards to the C++ implementation.
 * register convention: EAX -> color, ECX -> placement.
 * // blam-cc: EAX -> color, ECX -> placement
 *
 * @address 0x477670
 */
void object_placement_data_set_change_colors(real *color, object_placement_data *placement)
{
    halo::game::PlacementData(placement).set_change_colors(color);
}

}
