// actor_gather_nearby_grenade_targets  (Ghidra: actor_gather_nearby_grenade_targets, already named)
// address 0x42afc0, size 456 bytes
// name confidence: 0.8   rewrite confidence: 0.55
// evidence: Ghidra's own decompile silently dropped a third parameter -- the caller-owned
// output array actor_grenade_avoidance_entry_init (0x42af50) fills -- entirely, addressing
// it only through ESI at the two call sites; recovered from the real disassembly
// (objdump -d -M intel --start-address=0x42afc0 --stop-address=0x42b188 bin/halo.exe).
// The object-type test is decompiled as "(1 << object.type) & 1"; that is true only when
// object.type == 0 (_object_type_biped), which is what the rewrite tests directly.
// register convention: plain __cdecl, all three arguments on the stack.
// blam-cc: stack -> source_actor_index, maximum_count, out_entries

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"

extern data_array *actor_data;     // 0x00880360
extern data_array *encounter_data; // 0x008802c8
extern data_array *prop_data;      // 0x008802c0
extern data_array *object_data;    // 0x008603b0
extern ai_globals *ai_globals_ptr; // 0x00880354

extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, ECX actor, stack object
extern void actor_grenade_avoidance_entry_init(ai_grenade_avoidance_entry *entry,
                                                datum_index object_index,
                                                datum_index prop_index); // 0x0042af50

// blam-cc: stack -> source_actor_index, maximum_count, out_entries
// Collects up to maximum_count nearby friendly targets that a thrown grenade should make
// run: first every OTHER actor in source_actor's own encounter that has a unit and is not
// currently occupying a separate vehicle unit, then every recognized biped prop that is not
// itself owned by an actor already in that same encounter (so a squadmate counted by the
// first pass is not counted again by the second). Starts a grenade-avoidance timer entry
// for each one accepted and returns how many entries were filled.
int16_t actor_gather_nearby_grenade_targets(datum_index source_actor_index, int16_t maximum_count,
                                             ai_grenade_avoidance_entry *out_entries)
{
    actor *self;
    actor *other;
    datum_index cursor;
    prop *p;
    datum_index prop_cursor;
    datum_index next_prop;
    object *tracked_object;
    actor *prop_owner_actor;
    datum_index prop_index_out;
    int16_t count;

    self = &((actor *)actor_data->data)[source_actor_index & 0xffff];
    count = 0;
    cursor = self->encounter_index;

    if (cursor != (datum_index)k_datum_index_none) {
        if (ai_globals_ptr->actors_valid) {
            cursor = ((encounter *)encounter_data->data)[cursor & 0xffff].first_actor;
        }
        while (ai_globals_ptr->actors_valid && cursor != (datum_index)k_datum_index_none) {
            other = &((actor *)actor_data->data)[cursor & 0xffff];
            if (cursor != source_actor_index && count < maximum_count &&
                other->unit_index != (datum_index)k_datum_index_none &&
                other->active_unit_index == (datum_index)k_datum_index_none) {
                prop_index_out = actor_find_prop_for_object(other->unit_index, source_actor_index); // 0x42b04b: ECX = the source actor
                actor_grenade_avoidance_entry_init(&out_entries[count], other->unit_index, prop_index_out);
                count = count + 1;
            }
            cursor = other->next_in_encounter;
        }
    }

    prop_cursor = self->first_prop;
    while (prop_cursor != (datum_index)k_datum_index_none) {
        p = &((prop *)prop_data->data)[prop_cursor & 0xffff];
        next_prop = p->next_in_actor;

        if (p->is_unit == 0 && p->is_vault == 0 && p->kind == 3 &&
            p->relationship_object_index == -1) {
            tracked_object = ((object_header *)object_data->data)[p->object_index & 0xffff].data;
            if (tracked_object->type == _object_type_biped) {
                int excluded = 0;
                if (self->encounter_index != (datum_index)k_datum_index_none &&
                    p->owner_actor_index != (datum_index)k_datum_index_none) {
                    prop_owner_actor = &((actor *)actor_data->data)[p->owner_actor_index & 0xffff];
                    excluded = (prop_owner_actor->encounter_index == self->encounter_index);
                }
                if (!excluded && count < maximum_count) {
                    actor_grenade_avoidance_entry_init(&out_entries[count], p->object_index, prop_cursor);
                    count = count + 1;
                }
            }
        }
        prop_cursor = next_prop;
    }

    return count;
}

#if 0
Original Ghidra decompilation (0x42afc0):

short actor_gather_nearby_grenade_targets(uint param_1,short param_2)

{
  uint uVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  short sVar9;
  bool bVar10;
  uint local_4;

  iVar7 = (param_1 & 0xffff) * 0x724;
  iVar3 = *(int *)(DAT_00880360 + 0x34) + iVar7;
  uVar1 = *(uint *)(iVar3 + 0x34);
  sVar9 = 0;
  sVar2 = 0;
  if (uVar1 != 0xffffffff) {
    if (*(char *)(DAT_00880354 + 1) != '\0') {
      local_4 = *(uint *)((uVar1 & 0xffff) * 0x6c + 0x14 + *(int *)(DAT_008802c8 + 0x34));
    }
    while ((*(char *)(DAT_00880354 + 1) != '\0' && (local_4 != 0xffffffff))) {
      iVar4 = (local_4 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
      bVar10 = local_4 != param_1;
      local_4 = *(uint *)(iVar4 + 0x2c);
      if ((bVar10) &&
         (((sVar9 < param_2 && (*(int *)(iVar4 + 0x18) != -1)) && (*(int *)(iVar4 + 0x158) == -1))))
      {
        uVar5 = FUN_0043ea80(*(int *)(iVar4 + 0x18));
        FUN_0042af50(uVar5);
        sVar9 = sVar2 + 1;
        sVar2 = sVar9;
      }
    }
  }
  uVar1 = *(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x50 + iVar7);
  while (uVar8 = uVar1, uVar8 != 0xffffffff) {
    iVar7 = *(int *)(DAT_008802c0 + 0x34);
    iVar4 = (uVar8 & 0xffff) * 0x138;
    uVar1 = *(uint *)(iVar4 + 8 + iVar7);
    iVar6 = iVar4 + iVar7;
    if (((((*(char *)(iVar4 + 0x60 + iVar7) == '\0') && (*(char *)(iVar6 + 0x127) == '\0')) &&
         ((*(short *)(iVar6 + 0x24) == 3 &&
          ((*(int *)(iVar6 + 0x110) == -1 &&
           ((1 << (*(byte *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                     (*(uint *)(iVar6 + 0x18) & 0xffff) * 0xc) + 0xb4) & 0x1f) & 1U)
            != 0)))))) &&
        ((iVar7 = *(int *)(iVar3 + 0x34), iVar7 == -1 ||
         ((*(uint *)(iVar6 + 0x1c) == 0xffffffff ||
          (*(int *)((*(uint *)(iVar6 + 0x1c) & 0xffff) * 0x724 + 0x34 +
                   *(int *)(DAT_00880360 + 0x34)) != iVar7)))))) && (sVar9 < param_2)) {
      FUN_0042af50(uVar8);
      sVar9 = sVar2 + 1;
      sVar2 = sVar9;
    }
  }
  return sVar9;
}

Real disassembly (0x42afc0-0x42b187) used to recover the third (output-array) parameter and
the exact register mapping into actor_grenade_avoidance_entry_init:

0042afc0: sub    esp,0x14
0042afc3: mov    ecx,[esp+0x18]            ; ecx = source_actor_index (param_1)
0042afc7: mov    eax,ds:0x880360
0042afcc: mov    eax,[eax+0x34]
0042afcf: push   ebx
0042afd0: mov    ebx,ecx
0042afd2: and    ebx,0xffff
0042afd8: imul   ebx,ebx,0x724
0042afde: add    eax,ebx                   ; eax = self actor pointer
0042afe0: push   ebp
0042afe1: push   esi
0042afe2: mov    [esp+0x10],eax
0042afe6: mov    eax,[eax+0x34]            ; self->encounter_index
0042afe9: xor    esi,esi                   ; count = 0
0042afeb: cmp    eax,0xffffffff
0042afee: push   edi
0042afef: mov    [esp+0x10],esi
0042aff3: je     0x42b097
...
0042b059: mov    edi,[eax+0x18]            ; edi = other->unit_index
0042b05c: cmp    edi,0xffffffff
0042b05f: je     0x42b020
0042b061: cmp    dword ptr [eax+0x158],0xffffffff
0042b068: jne    0x42b020
0042b06a: movsx  eax,si
0042b06d: lea    edx,[eax+eax*4]
0042b070: mov    eax,[esp+0x30]            ; eax = out_entries (the missing 3rd parameter)
0042b074: push   edi
0042b075: lea    esi,[eax+edx*8]           ; esi = &out_entries[count]
0042b078: call   0x43ea80
0042b07d: push   eax
0042b07e: call   0x42af50                  ; ESI=entry, EDI=object_index, stack=prop_index
...
0042b0ed: mov    edi,[eax+0x18]            ; edi = p->object_index
...
0042b157: mov    ecx,[esp+0x30]            ; ecx = out_entries
0042b15b: movsx  eax,si
0042b15e: lea    eax,[eax+eax*4]
0042b161: lea    esi,[ecx+eax*8]           ; esi = &out_entries[count]
0042b164: push   edx                       ; edx = prop_cursor
0042b165: call   0x42af50
#endif
