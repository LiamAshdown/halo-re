// network_map_cycle_list_broadcast  (Ghidra: FUN_004deec0; named per this rewrite)
// address 0x4deec0, size 190 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Builds and broadcasts a message (type 0x35)
// listing every entry returned by data_iterator_next, e.g. the map/variant cycle list, via
// FUN_004ec940/FUN_004e19c0." Up to 16 entries, each an 8-byte (byte, dword) record staged into
// the static scratch array at 0x00861d60.
// UNSURE: data_iterator_next's own `data_iterator *iterator` argument is elided at both call
// sites; a NULL/zeroed local is used here so the iterator can advance from a fixed starting
// point, but the real source iterator (presumably a tag iteration over the map/variant list)
// could not be identified.
// UNSURE: the original loops unconditionally while the iterator keeps returning items, writing
// into the static global scratch array at 0x00861d60 (which may have had headroom beyond the 16
// entries `local_440` can index); this rewrite uses local stack arrays instead and adds a
// `count < 16` bound to avoid overflowing them, which is a defensive deviation from the literal
// decompile rather than an observed limit.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>
#include "objects.h"
#include "units.h"
#include "items.h"
#include "fn_networking.h"


extern network_server_globals *network_server; // 0x0071c2d4
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module
extern int32_t message_delta_encode_message(int32_t a, int32_t type, int32_t b, void **entries,
    int32_t c, int32_t count, char d); // 0x4ec940, outside this batch
extern char network_session_broadcast_to_all(network_server_globals *server, int32_t param_1,
    void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6);
    // blam-cc: ECX -> server, stack -> param_1, data, param_3, param_4, force, param_6;
    // this module, 0x4e19c0

void network_map_cycle_list_broadcast(void)
{
    void *entries[16];
    uint8_t encoded[1024];
    network_map_cycle_entry scratch[16];
    int32_t count;
    data_iterator iterator;
    void *item; // data_iterator_next returns the element pointer (src/memory)

    for (count = 0; count < 16; count++) {
        entries[count] = 0;
    }
    count = 0;
    iterator.data = 0;
    iterator.next_index = 0;
    iterator.index = 0;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    item = data_iterator_next(&iterator); // UNSURE: iterator source elided, see header
    if (item != 0) {
        do {
            scratch[count].unknown_00 = *(uint8_t *)((uint8_t *)item + 0x67);
            scratch[count].unknown_04 = *(uint32_t *)&((struct item_object *)item)->base.maximum_shield_vitality;
            entries[count] = &scratch[count];
            count = count + 1;
            item = data_iterator_next(&iterator);
        } while (item != 0 && count < 16);
        if (count > 0) {
            message_delta_encode_message(0, 0x35, 0, entries, 0, count, 0);
            network_session_broadcast_to_all(network_server, 1, encoded, 0, 0, 0, 3);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4deec0):

void FUN_004deec0(void)

{
  int iVar1;
  int count;
  undefined1 *puVar2;
  void **ppvVar3;
  void *local_440 [16];
  undefined1 local_400 [1024];

  ppvVar3 = local_440;
  for (iVar1 = 0x10; iVar1 != 0; iVar1 = iVar1 + -1) {
    *ppvVar3 = (void *)0x0;
    ppvVar3 = ppvVar3 + 1;
  }
  count = 0;
  iVar1 = data_iterator_next();
  if (iVar1 != 0) {
    puVar2 = &DAT_00861d60;
    do {
      *puVar2 = *(undefined1 *)(iVar1 + 0x67);
      *(undefined4 *)(puVar2 + 4) = *(undefined4 *)(iVar1 + 0xdc);
      local_440[count] = puVar2;
      count = count + 1;
      puVar2 = puVar2 + 8;
      iVar1 = data_iterator_next();
    } while (iVar1 != 0);
    if (0 < count) {
      message_delta_encode_message(0,0x35,0,local_440,0,count,'\0');
      FUN_004e19c0(1,local_400,0,0,0,3);
    }
  }
  return;
}
#endif
