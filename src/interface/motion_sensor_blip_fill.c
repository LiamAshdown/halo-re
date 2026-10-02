// motion_sensor_blip_fill  (Ghidra: motion_sensor_blip_fill, already named)
// address 0x4b35f0, size 110 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: objdump 0x4b35f0..0x4b365d. The type is blip_type_get(local player, the object in
// ECX, moved to EBX); the first rewrite passed an extra unused argument as the object. The
// subtype is the Unit tag short at +0x298 of a live unit when it is 0..2, else 0.
// register convention: EAX local player index, ECX object index, ESI blip.
//   // blam-cc: EAX -> local_player_index, ECX -> object_index, ESI -> blip

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern uint8_t blip_type_get(int16_t local_player_index, datum_index object_index); // 0x4b3450, blam-cc: EBX object_index

// blam-cc: EAX -> local_player_index, ECX -> object_index, ESI -> blip
// FIXED (register inputs, objdump): note phrasing only -- rewritten from the reversed
// "name -> REG" form (and "object" for the object_index parameter) the checker cannot parse.
void motion_sensor_blip_fill(int16_t local_player_index, datum_index object_index, motion_sensor_blip *blip)
{
    blip->type = blip_type_get(local_player_index, object_index);
    if (object_index != (datum_index)-1 && object_try_and_get(object_index, 3) != 0) {
        uint8_t *object_ptr = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
        int16_t subtype = *(int16_t *)((uint8_t *)tag_instances[*(datum_index *)object_ptr & 0xffff].data + 0x298);

        blip->subtype = (subtype >= 0 && subtype < 3) ? (uint8_t)subtype : 0;
        return;
    }
    blip->subtype = 0;
}

#if 0
Original Ghidra decompilation (0x4b35f0):

void FUN_004b35f0(void)

{
  undefined1 uVar1;
  short sVar2;
  int iVar3;
  uint in_ECX;
  int unaff_ESI;

  uVar1 = blip_type_get();
  *(undefined1 *)(unaff_ESI + 2) = uVar1;
  if (in_ECX != 0xffffffff) {
    iVar3 = object_try_and_get(3);
    if (iVar3 != 0) {
      sVar2 = *(short *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                              (in_ECX & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                                 DAT_0087bc14) + 0x298);
      if ((sVar2 < 0) || (2 < sVar2)) {
        *(undefined1 *)(unaff_ESI + 3) = 0;
        return;
      }
      goto LAB_004b3659;
    }
  }
  sVar2 = 0;
LAB_004b3659:
  *(char *)(unaff_ESI + 3) = (char)sVar2;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
