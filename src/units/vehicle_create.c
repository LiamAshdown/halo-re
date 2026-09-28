// vehicle_create  (not a Ghidra function; the vehicle type's +0x28 (create) callback)
// address 0x570bb0, size 198 bytes
// name confidence: 0.65  rewrite confidence: 0.9
// evidence: object_type_definition vehicle (0x0069b5b8) field +0x28 (create); the object type dispatch (object_type_definitions_*)
//   calls it cdecl with the object handle. Only reachable through that table (never a Ghidra function);
//   first-boot track: needed while placing the UI map's objects.
//   objdump 0x570bb0..0x570c75: vehicle_reset_state; flag 0x20 set when the vehicle tag's +0x8c is -1 and
//   cleared otherwise; when it is not -1 the object's +0x64 grows by half the tag's +0x04; in a single-player or
//   multiplayer map +0x525..+0x527 and +0x9 are cleared; +0x5ac = the game time's +0xc; +0x5b4 = the position
//   (+0x5c); +0x524 = 0. The result is 1.
// blam-cc: stack -> object_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

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

extern int16_t map_difficulty_or_kind; // 0x00719720
extern game_time_globals *game_time; // 0x006f1d6c
extern void vehicle_reset_state(uint32_t object_index); // 0x570b00

uint8_t vehicle_create(datum_index object_index)
{
    uint8_t *object = object_get(object_index);
    uint8_t *definition = object_definition(object);
    int32_t i;

    vehicle_reset_state(object_index);
    if (*(int32_t *)(definition + 0x8c) == -1) {
        *(uint32_t *)(object + 0x10) |= 0x20;
    } else {
        *(uint32_t *)(object + 0x10) &= ~(uint32_t)0x20;
        *(float *)(object + 0x64) += *(float *)(definition + 4) * 0.5f;
    }
    if (map_difficulty_or_kind == 1 || map_difficulty_or_kind == 2) {
        object[0x525] = 0;
        object[0x526] = 0;
        object[0x527] = 0;
        object[0x9] = 0;
    }
    *(uint32_t *)(object + 0x5ac) = (uint32_t)game_time->game_time;
    for (i = 0; i < 3; i++) {
        ((uint32_t *)(object + 0x5b4))[i] = ((uint32_t *)(object + 0x5c))[i];
    }
    object[0x524] = 0;
    return 1;
}
