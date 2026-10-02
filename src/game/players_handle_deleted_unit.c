// players_handle_deleted_unit  (not a Ghidra function; object_delete_callbacks[2])
// address 0x479e20, size 133 bytes
// name confidence: 0.6  rewrite confidence: 0.95
// evidence: the third entry of object_delete_callbacks (0x0069b354: 0x4f73e0, 0x42c140, 0x479e20), called for every
//   object being deleted; only reachable through that table. Campaign track: objects_update deleted a unit.
// objdump 0x479e20..0x479ea4: for a unit (object type bit 0 or 1, +0xb4), every player (player_data 0x0087a480,
//   walked with a data_iterator) whose unit (+0x34) is this object is reset with
//   player_reset_after_unit_change(stack: the player handle).
// blam-cc: stack -> object_index (cdecl)

#include "tags.h"
#include "memory.h"
#include <stdint.h>
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI
extern void player_reset_after_unit_change(uint32_t player_index); // 0x474e10

void players_handle_deleted_unit(uint32_t object_index)
{
    uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
    data_iterator iterator;
    uint8_t *player;

    if (((1u << (object[0xb4] & 0x1f)) & 3) == 0) {
        return;
    }
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)player_data ^ k_data_iterator_signature;
    for (player = (uint8_t *)data_iterator_next(&iterator); player != 0;
         player = (uint8_t *)data_iterator_next(&iterator)) {
        if (*(uint32_t *)&((struct player *)player)->unit == object_index) {
            player_reset_after_unit_change(iterator.index);
        }
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
