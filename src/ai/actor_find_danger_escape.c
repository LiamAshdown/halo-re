// actor_find_danger_escape  (not a Ghidra function; no C existed, so a call would have hit a trap)
// address 0x40bc40, size 1011 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// WRITTEN from objdump 0x40bc40..0x40c033.
//   Pick a sideways dodge from a danger (a grenade: position +0x2b0, velocity +0x2bc, path end +0x2c8, radius +0x294)
//   for an actor whose unit tag has a dodge step (+0x238): the dodge axis is away from the danger's motion (or
//   from the danger itself, or the facing, or the global forward). Both sideways steps (left, right) are tested
//   (0x417bb0, sideways reach 8 with Actor flag 0x2000000) and measured against the danger's path (0x4cde30). The
//   clear side outside the blast wins (direction kind 0 / 1); with both open the farther one by more than 0.3, else
//   kind 4 (either). Outputs: the kind (word, -1 none), the step (float), the dodge axis (vec2), whether the chosen
//   step was blocked. Returns whether the dodge gets out of the blast.
// blam-cc: stack -> (actor_index, out_direction_kind, out_step, out_direction, out_blocked)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "cache.h"
#include "fn_ai.h"
#include "fn_math.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern const real_vector2d *global_forward2d_pointer; // 0x006966e8

extern double sqrt(double x);
extern double fabs(double x);


extern real point3d_distance_squared_to_segment(real_point3d *segment_start, real_vector3d *segment_direction,
                                                real_point3d *point); // 0x4cde30, EAX, ECX, EDX

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

uint8_t actor_find_danger_escape(datum_index actor_index, uint32_t *out_word, uint8_t *out_position,
                                 real_vector3d *path_delta, uint8_t *in_danger)
{
    uint8_t *act = ACTOR(actor_index);
    uint8_t *unit_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(((actor *)act)->unit_index));
    float step = *(float *)(unit_tag + 0x238);
    int16_t kind = -1;
    uint8_t blocked = 0;
    uint8_t escapes = 0;
    real_vector2d axis = {0.0f, 0.0f};

    if (step > 0.0f) {
        uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
        float sideways = (*(uint32_t *)actor_tag & 0x2000000) ? 8.0f : 0.0f;
        float length;
        real_vector3d path;
        real_vector3d left;
        real_vector3d right;
        real_point3d left_point;
        real_point3d right_point;
        uint8_t left_blocked = 0;
        uint8_t right_blocked = 0;
        uint8_t left_hit;
        uint8_t right_hit;
        uint8_t left_out;
        uint8_t right_out;
        float left_distance;
        float right_distance;
        uint8_t extra[0x30];
        uint8_t have_axis = 0;

        axis.i = -*(float *)(act + 0x2bc);
        axis.j = -*(float *)(act + 0x2c0);
        length = (float)sqrt(axis.j * axis.j + axis.i * axis.i);
        if (fabs(length) >= 9.999999747378752e-05) {
            float inverse = 1.0f / length;

            axis.i *= inverse;
            axis.j *= inverse;
            if (length > 0.033333335f) {
                have_axis = 1;
            }
        }
        if (!have_axis) {
            axis.i = ((actor *)act)->flee_from_point.x - ((actor *)act)->body_position.x;
            axis.j = ((actor *)act)->flee_from_point.y - ((actor *)act)->body_position.y;
            if (vector2d_normalize_with_length(&axis) == 0.0f) {
                axis.i = ((actor *)act)->facing.i;
                axis.j = ((actor *)act)->facing.j;
                if (vector2d_normalize_with_length(&axis) == 0.0f) {
                    axis = *global_forward2d_pointer;
                }
            }
        }
        path.i = ((actor *)act)->danger_segment_end.x - ((actor *)act)->flee_from_point.x;
        path.j = ((actor *)act)->danger_segment_end.y - ((actor *)act)->flee_from_point.y;
        path.k = ((actor *)act)->danger_segment_end.z - ((actor *)act)->flee_from_point.z;
        left.i = -axis.j;
        left.j = axis.i;
        left.k = 0.0f;
        right.i = axis.j;
        right.j = -axis.i;
        right.k = 0.0f;
        left_point.x = left.i * step + ((actor *)act)->body_position.x;
        left_point.y = axis.i * step + ((actor *)act)->body_position.y;
        left_point.z = step * 0.0f + ((actor *)act)->body_position.z;
        right_point.x = axis.j * step + ((actor *)act)->body_position.x;
        right_point.y = right.j * step + ((actor *)act)->body_position.y;
        right_point.z = step * 0.0f + ((actor *)act)->body_position.z;

        left_hit = actor_check_step_obstruction(actor_index, (real_vector2d *)&left, step, sideways, &left_blocked, extra);
        left_distance = (float)sqrt(point3d_distance_squared_to_segment((real_point3d *)(act + 0x2b0), &path, &left_point));
        left_out = (uint8_t)(left_hit && left_distance > ((actor *)act)->danger_unknown_294);
        right_hit = actor_check_step_obstruction(actor_index, (real_vector2d *)&right, step, sideways, &right_blocked, extra);
        right_distance = (float)sqrt(point3d_distance_squared_to_segment((real_point3d *)(act + 0x2b0), &path, &right_point));
        right_out = (uint8_t)(right_hit && right_distance > ((actor *)act)->danger_unknown_294);

        if (left_hit) {
            if (right_hit) {
                float difference = left_distance - right_distance;

                if (left_blocked > right_blocked || left_out > right_out || difference > 0.3f) {
                    kind = 0;
                    escapes = 1;
                    blocked = left_blocked;
                } else if (right_blocked > left_blocked || right_out > left_out || difference < -0.3f) {
                    kind = 1;
                    escapes = 1;
                    blocked = right_blocked;
                } else {
                    kind = 4;
                    escapes = left_out;
                    blocked = left_blocked;
                }
            } else {
                kind = 0;
                blocked = left_blocked;
                escapes = left_out;
            }
        } else if (right_hit) {
            kind = 1;
            escapes = right_out;
            blocked = right_blocked;
        }
    }
    *(int16_t *)out_word = kind;
    *(float *)out_position = step;
    *in_danger = blocked;
    path_delta->i = axis.i;
    path_delta->j = axis.j;
    return escapes;
}
