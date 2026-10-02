// actor_mode_converse_exit  (not a Ghidra function: actor mode table 0x65524c, mode "converse" slot +0x20)
// address 0x402f40, size 51 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN from objdump 0x402f40..0x402f73 (no C existed: a mode entered in game would have hit a trap).
//   Conversing ends: the actor's conversation (+0x1dc) is stopped (0x430ea0 with reasons 0, 0).
// blam-cc: stack -> actor_index (cdecl, called through the mode table)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

extern void ai_conversation_stop(datum_index instance_handle, uint8_t reason_a, uint8_t reason_b); // 0x430ea0

void actor_mode_converse_exit(datum_index actor_index)
{
    datum_index conversation = *(datum_index *)(ACTOR(actor_index) + 0x1dc);

    if (conversation != k_datum_index_none) {
        ai_conversation_stop(conversation, 0, 0);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
