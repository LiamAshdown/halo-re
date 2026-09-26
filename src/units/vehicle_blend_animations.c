// vehicle_blend_animations  (not a Ghidra function: Ghidra folded it into vehicle_update; it follows that
//   function's switch table at 0x5718d0)
// address 0x5718e0, size 912 bytes
// name confidence: 0.5  rewrite confidence: 0.85
// evidence: the vehicle object_type_definition (0x0069b5b8) +0x48 slot 0x0069b600 holds 0x5718e0; object
//   node orientation building calls it through object_type_definitions_notify_two_args_0x48 with the object and
//   the orientation buffer. Only reachable through that slot. First-boot track: a campaign level's vehicles.
// objdump 0x5718e0..0x571c6f: with an animation graph on the Vehicle tag (+0x44) that has a vehicle block (+0x24
//   count, +0x28 first entry), the entry's animation list (+0x5c count, +0x60 int16 indices) drives:
//   [0] animation_aiming_screen_blend(EDI = animation, stack: the entry, yaw = object +0x4dc, pitch 0, out);
//   [1] speed: (scalar_triple_product(a = object +0x80, b = +0x74, c = +0x68) / tag +0x2f8 + 1) / 2, clamped 0..1;
//   [2] steering: object +0x4d4 below zero gives 0.5 - s / tag +0x2fc / 2, else (s / tag +0x2f8 + 1) / 2;
//   [3] throttle: the velocity (+0x68) dot the forward (+0x74) clamped 0..1, divided by |tag +0x2f8|, clamped;
//   [5] with tag +0x310 above zero, object +0x4e0 / tag +0x310 (else 0), scaled by the full frame count;
//   the others by frame count - 1, each through animation_overlay_interpolated_frame_orientations(EDI =
//   animation, stack: frame, out). Then every suspension entry (+0x68 count, 0x14 each at +0x6c, animation at
//   +2) blends at object +0x4f4 + i / 255 (0xff meaning 1).
// blam-cc: stack -> object_index, orientations (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "models.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern float vector3d_scalar_triple_product(const real_vector3d *a, const real_vector3d *b, const real_vector3d *c);
    // 0x44d8e0, blam-cc: EAX -> b, EDX -> c, stack -> a
extern void animation_overlay_interpolated_frame_orientations(ModelAnimationsAnimation *animation, float frame,
    real_orientation *out_orientations); // 0x4d53f0, blam-cc: EDI -> animation, stack -> (frame, out)
extern void animation_aiming_screen_blend(ModelAnimationsAnimation *animation, animation_aiming_screen *screen,
    real yaw, real pitch, real_orientation *orientation_out);
    // 0x4d5c00, blam-cc: EDI -> animation, stack -> screen, yaw, pitch, orientation_out
extern double fabs(double x);

static double clamp_unit(double value)
{
    if (value < 0.0) {
        return 0.0;
    }
    if (value > 1.0) {
        return 1.0;
    }
    return value;
}

// Blends `animation` at `fraction` of its last frame (frame count - 1).
static void blend_fraction(ModelAnimationsAnimation *animation, double fraction, real_orientation *orientations)
{
    int32_t last_frame = *(int16_t *)((uint8_t *)animation + 0x22) - 1;

    animation_overlay_interpolated_frame_orientations(animation, (float)((double)last_frame * fraction), orientations);
}

void vehicle_blend_animations(datum_index object_index, real_orientation *orientations)
{
    uint8_t *obj = *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
    uint8_t *vehicle_tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    datum_index graph_tag = *(datum_index *)(vehicle_tag + 0x44);
    uint8_t *graph;
    uint8_t *entry;
    uint8_t *animations;
    int32_t count;
    int16_t *indices;
    int16_t i;

    if (graph_tag == k_datum_index_none) {
        return;
    }
    graph = (uint8_t *)tag_instances[graph_tag & 0xffff].data;
    if (*(int32_t *)(graph + 0x24) == 0) {
        return;
    }
    entry = *(uint8_t **)(graph + 0x28);
    if (entry == 0) {
        return;
    }
    animations = *(uint8_t **)(graph + 0x78);
    count = *(int32_t *)(entry + 0x5c);
    indices = *(int16_t **)(entry + 0x60);

    if (count > 0 && indices[0] != -1) {
        animation_aiming_screen_blend((ModelAnimationsAnimation *)(animations + indices[0] * 0xb4),
            (animation_aiming_screen *)entry, *(real *)(obj + 0x4dc), 0.0f, orientations);
    }
    if (count > 1 && indices[1] != -1) {
        double speed = vector3d_scalar_triple_product((real_vector3d *)(obj + 0x80), (real_vector3d *)(obj + 0x74),
            (real_vector3d *)(obj + 0x68));

        speed = (speed / *(float *)(vehicle_tag + 0x2f8) + 1.0) * 0.5;
        blend_fraction((ModelAnimationsAnimation *)(animations + indices[1] * 0xb4), clamp_unit(speed), orientations);
    }
    if (count > 2 && indices[2] != -1) {
        float steering = *(float *)(obj + 0x4d4);
        double fraction;

        if (steering < 0.0f) {
            fraction = 0.5 - steering / *(float *)(vehicle_tag + 0x2fc) * 0.5;
        } else {
            fraction = (steering / *(float *)(vehicle_tag + 0x2f8) + 1.0) * 0.5;
        }
        blend_fraction((ModelAnimationsAnimation *)(animations + indices[2] * 0xb4), fraction, orientations);
    }
    if (count > 3 && indices[3] != -1) {
        double forward_speed = (double)*(float *)(obj + 0x70) * *(float *)(obj + 0x7c) +
            (double)*(float *)(obj + 0x6c) * *(float *)(obj + 0x78) +
            (double)*(float *)(obj + 0x68) * *(float *)(obj + 0x74);

        forward_speed = clamp_unit(clamp_unit(forward_speed) / fabs(*(float *)(vehicle_tag + 0x2f8)));
        blend_fraction((ModelAnimationsAnimation *)(animations + indices[3] * 0xb4), forward_speed, orientations);
    }
    if (count > 5 && indices[5] != -1) {
        ModelAnimationsAnimation *animation = (ModelAnimationsAnimation *)(animations + indices[5] * 0xb4);
        double fraction = 0.0;
        int32_t frames = *(int16_t *)((uint8_t *)animation + 0x22);

        if (*(float *)(vehicle_tag + 0x310) > 0.0f) {
            fraction = *(float *)(obj + 0x4e0) / *(float *)(vehicle_tag + 0x310);
        }
        animation_overlay_interpolated_frame_orientations(animation, (float)((double)frames * fraction), orientations);
    }
    for (i = 0; i < *(int32_t *)(entry + 0x68); i++) {
        int16_t suspension = *(int16_t *)(*(uint8_t **)(entry + 0x6c) + i * 0x14 + 2);

        if (suspension != -1) {
            uint8_t compression = obj[0x4f4 + i];
            double fraction = compression == 0xff ? 1.0 : (double)compression * (double)0.0039215689f; // 0x672ad4

            blend_fraction((ModelAnimationsAnimation *)(animations + suspension * 0xb4), fraction, orientations);
        }
    }
}
