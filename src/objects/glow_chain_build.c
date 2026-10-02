// glow_chain_build
// address 0x4fd830, size 166 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md misattribution table: "0x4fd830 |
//   lightning_bolt_build_chain | glow_chain_build")
// rewrite confidence: 0.5
// evidence: types/objects.h glow (definition_tag 0x224, spawn_count 0x24c, first_particle
//   0x250, last_particle 0x254), glow_particle (flags 0x54, next 0x5c, previous 0x60);
//   types/tags.h Glow.glow_flags bits 0x02/0x04 tested here.
// register convention: Ghidra shows a single `int in_EAX` with no other implicit inputs; by
//   analogy with glow_particle_compute_fade.c's EAX=entry convention, EAX is the glow entry.
// blam-cc: EAX -> entry

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14

extern glow_particle *glow_particle_new(glow *entry, int16_t index, int16_t count); // this module, 0x4fd8e0

void glow_chain_build(glow *entry /*EAX*/) // blam-cc: EAX -> entry
{
    uint8_t *tag = (uint8_t *)tag_instances[entry->definition_tag & 0xffff].data;
    int32_t i = 0;
    int alternate = 1;
    glow_particle *prev = 0;

    for (i = 0; i < entry->spawn_count; i++) {
        glow_particle *p = glow_particle_new(entry, (int16_t)i, entry->spawn_count);
        if (p == 0) {
            return;
        }

        if ((tag[0x28] & 2) != 0) {
            ((struct glow_particle *)p)->flags |= 1;
        }
        if ((tag[0x28] & 4) != 0) {
            uint32_t flags = ((struct glow_particle *)p)->flags;
            if (alternate) {
                flags &= ~1U;
            } else {
                flags |= 1;
            }
            alternate = !alternate;
            ((struct glow_particle *)p)->flags = flags;
        }

        if (entry->first_particle == 0) {
            entry->first_particle = p;
        }
        if (prev != 0) {
            *(glow_particle **)&((struct glow_particle *)prev)->next = p;
        }
        *(glow_particle **)&((struct glow_particle *)p)->previous = prev;
        entry->last_particle = p;
        prev = p;
    }
}

#if 0
Original Ghidra decompilation (0x4fd830):

void FUN_004fd830(void)

{
  int iVar1;
  bool bVar2;
  int in_EAX;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;

  iVar1 = *(int *)((*(uint *)(in_EAX + 0x224) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar5 = 0;
  bVar2 = true;
  iVar6 = 0;
  if (0 < *(short *)(in_EAX + 0x24c)) {
    do {
      iVar3 = FUN_004fd8e0(iVar5,*(undefined2 *)(in_EAX + 0x24c));
      if (iVar3 == 0) {
        return;
      }
      if ((*(byte *)(iVar1 + 0x28) & 2) != 0) {
        *(uint *)(iVar3 + 0x54) = *(uint *)(iVar3 + 0x54) | 1;
      }
      if ((*(byte *)(iVar1 + 0x28) & 4) != 0) {
        if (bVar2) {
          uVar4 = *(uint *)(iVar3 + 0x54) & 0xfffffffe;
        }
        else {
          uVar4 = *(uint *)(iVar3 + 0x54) | 1;
        }
        bVar2 = !bVar2;
        *(uint *)(iVar3 + 0x54) = uVar4;
      }
      if (*(int *)(in_EAX + 0x250) == 0) {
        *(int *)(in_EAX + 0x250) = iVar3;
      }
      if (iVar6 != 0) {
        *(int *)(iVar6 + 0x5c) = iVar3;
      }
      *(int *)(iVar3 + 0x60) = iVar6;
      iVar5 = iVar5 + 1;
      *(int *)(in_EAX + 0x254) = iVar3;
      iVar6 = iVar3;
    } while ((short)iVar5 < *(short *)(in_EAX + 0x24c));
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
