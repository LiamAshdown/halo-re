// biped_create  (not a Ghidra function; the biped type's +0x28 (create) callback)
// address 0x558dc0, size 233 bytes
// name confidence: 0.65  rewrite confidence: 0.9
// evidence: object_type_definition biped (0x0069b4f0) field +0x28 (create); the object type dispatch (object_type_definitions_*)
//   calls it cdecl with the object handle. Only reachable through that table (never a Ghidra function);
//   first-boot track: needed while placing the UI map's objects.
//   objdump 0x558dc0..0x558ea8: the resting plane (+0x514) starts as the default plane at 0x0069c53c; +0x504 =
//   0x7f; the handles at +0x4d8/+0x4dc/+0x4ec/+0x4f0/+0x4f4 and +0x4d4 are -1; the position goes to +0x4e0
//   (object_get_position); bipeds whose tag flags (+0x2f4) have bit 6 find their surface plane; the up vector is
//   updated; +0x4d3 = 0; in a single-player or multiplayer map (0x00719720 1 or 2) +0x526..+0x528 and +0x9 are
//   cleared; +0x122 = 0. The result is 1.
// blam-cc: stack -> object_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}

static uint8_t *object_definition(uint8_t *object)
{
    return (uint8_t *)tag_instances[*(datum_index *)object & 0xffff].data;
}

extern int16_t network_game_mode; // 0x00719720
extern uint32_t k_default_resting_plane[4]; // 0x0069c53c
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX out, ECX object
extern void unit_find_nearest_valid_surface_plane(uint32_t unit_index); // 0x560630, ECX unit
extern void unit_update_up_vector(void *biped_tag, void *obj); // 0x560800, EAX tag, ECX object

uint8_t biped_create(datum_index object_index)
{
    uint8_t *object = object_get(object_index);
    uint8_t *definition = object_definition(object);
    int32_t i;

    for (i = 0; i < 4; i++) {
        ((uint32_t *)(object + 0x514))[i] = k_default_resting_plane[i];
    }
    object[0x504] = 0x7f;
    *(int32_t *)(object + 0x4d8) = -1;
    *(int32_t *)(object + 0x4dc) = -1;
    object_get_position((real_point3d *)(object + 0x4e0), object_index);
    *(int32_t *)(object + 0x4f0) = -1;
    *(int32_t *)(object + 0x4ec) = -1;
    *(int32_t *)(object + 0x4f4) = -1;
    if ((definition[0x2f4] & 0x40) != 0) {
        unit_find_nearest_valid_surface_plane(object_index);
    }
    unit_update_up_vector(definition, object);
    object[0x4d3] = 0;
    *(int32_t *)(object + 0x4d4) = -1;
    if (network_game_mode == 1 || network_game_mode == 2) {
        object[0x526] = 0;
        object[0x527] = 0;
        object[0x528] = 0;
        object[0x9] = 0;
    }
    object[0x122] = 0;
    return 1;
}
