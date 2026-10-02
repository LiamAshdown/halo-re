// recorded_animations_update  (Ghidra: recorded_animations_update, already named)
// address 0x44aa90, size 550 bytes
// name confidence: 0.6   rewrite confidence: 0.9 (verified against objdump 0x44aa90..0x44acb5)
// evidence: types/cutscene.h recorded_animation struct comment ("ticked by 0x44aa90") and
//   types/units.h biped_data.flags comment: "bit 1 = jumping (0x559fa0 sets 0 and 1 together;
//   the recorded-animation update also sets it, or BYTE [edi+0x4cc],0x2 at 0x44ac86)" -- the
//   units module's own type recovery already names and cross-references this exact store.
//   Confirmed field-by-field via
//   `objdump -d -M intel --start-address=0x44aa90 --stop-address=0x44ad20 bin/halo.exe`, which
//   also showed the object-handle validation at the top of the loop (identifier/salt check
//   against object_data, then `shl 1,type; test dl,3`) is byte-for-byte
//   src/objects/object_try_and_get.c's own logic with type_mask 3 (bit 0 biped, bit 1 vehicle);
//   this rewrite calls that function instead of re-inlining it.
// register convention: no arguments; builds its own local data_iterator over
//   recorded_animations (same layout as its siblings recorded_animation_find_by_object /
//   recorded_animation_object_is_playing). No return value.
// UNSURE: object + 0x4cc and object + 0x4fc are the shared leading bytes of biped_data and
//   vehicle_data (types/units.h: "biped_data and vehicle_data both start at 0x4cc... the same
//   byte offset means two different things depending on the object type"), and this function
//   writes them through raw object offsets without a type-specific struct, exactly as the
//   original binary does (it only ever validated biped-or-vehicle, never which one) -- kept as
//   raw pointer arithmetic here rather than picking one of the two extension structs.
// UNSURE: hs_object_hierarchy_test's real parameter meaning (it is only ever used here as a
//   plain bool gate) and FUN_00474db0-style siblings' exact semantics are not independently
//   re-derived in this file.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "cutscene.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *recorded_animations;     // 0x006b0a10
extern data_array *object_data;             // 0x008603b0
extern recorded_animation_codec *recorded_animation_codecs_by_version[4]; // 0x00686fe8

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, blam-cc: EAX -> array, EDX -> handle
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, objects module, blam-cc: ECX -> object_index, stack -> type_mask
extern void unit_apply_control_block(uint32_t unit_index, const unit_control_data *control, int32_t source_id); // 0x5639f0, units module
extern void unit_refresh_targeting_flag_and_weapons(datum_index unit_handle, uint8_t attaching); // 0x569bf0, units module
    // blam-cc: stack -> unit_handle, CL -> attaching
extern char hs_object_hierarchy_test(datum_index object_index); // 0x487c10, hs module
extern void object_delete(datum_index object_index); // 0x4f5bd0, objects module, blam-cc: EAX -> object_index
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, objects module, blam-cc: EAX -> out, ECX -> object_index

// Ticks every live recorded_animation once: validates its unit (a biped or vehicle only,
// type_mask 3), deletes the record if the unit is gone or of the wrong type, otherwise either
// tears it down (when its finished flag is set: restores the biped "jumping"/PVS flags, unlinks
// from device group tracking, optionally deletes the unit if it is no longer part of the object
// hierarchy, optionally re-marks the unit's cached position, then deletes the record) or
// advances it one tick (runs its codec's update, applies the decoded control block to the unit,
// and sets or clears the finished flag from the codec's return).
void recorded_animations_update(void)
{
    data_iterator iterator;
    recorded_animation *record;

    iterator.data = recorded_animations;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    record = (recorded_animation *)data_iterator_next(&iterator);
    while (record != (recorded_animation *)0) {
        object *unit_object = object_try_and_get(record->unit_index, 0x3);

        if (unit_object == (object *)0) {
            datum_delete(recorded_animations, iterator.index);
        } else if ((record->flags & _recorded_animation_flag_finished) != 0) {
            object_header *header = &((object_header *)object_data->data)[record->unit_index & 0xffff];
            unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);

            if ((record->flags & _recorded_animation_flag_restore_object_flag_40) != 0) {
                unit->flags = unit->flags | 0x40; // bit not named in types/units.h unit_flags
            } else {
                unit->flags = unit->flags & ~0x00000040u;
            }
            header = &((object_header *)object_data->data)[record->unit_index & 0xffff];
            unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);
            unit->flags = unit->flags & ~0x08000000u; // _unit_flag_unknown_8000000

            unit_refresh_targeting_flag_and_weapons(record->unit_index, 0);

            header = &((object_header *)object_data->data)[record->unit_index & 0xffff];
            header->flags = header->flags | _object_header_in_pvs_pass_bit;
            // (orphan pass 4 review: the draft also set object + 0x4cc bit 1 here; objdump
            // 0x44abe7..0x44ac34 has no such store -- the only 0x4cc write is the
            // mark_object_when_finished one at 0x44ac86 below.)
            if ((header->data->parent_object == (datum_index)k_datum_index_none) &&
                (header->data->location_cluster_index == -1)) {
                if ((header->flags & _object_header_active_bit) != 0) {
                    header->flags = header->flags & ~(uint8_t)_object_header_active_bit;
                }
            }

            if (((record->flags & 0x08) != 0) && (record->unit_index != (datum_index)k_datum_index_none)) {
                if (hs_object_hierarchy_test(record->unit_index) == 0) {
                    object_delete(record->unit_index);
                }
            }
            if (((record->flags & 0x10) != 0) && (record->unit_index != (datum_index)k_datum_index_none)) {
                object *obj = ((object_header *)object_data->data)[record->unit_index & 0xffff].data;
                object_get_position((real_point3d *)((uint8_t *)obj + 0x4fc), record->unit_index);
                *((uint8_t *)obj + 0x4cc) = *((uint8_t *)obj + 0x4cc) | 0x02;
            }
            datum_delete(recorded_animations, iterator.index);
        } else {
            recorded_animation_codec *codec;
            uint8_t not_finished;

            record->ticks_remaining = record->ticks_remaining - 1;
            codec = recorded_animation_codecs_by_version[record->codec_index];
            not_finished = codec->update(&record->decoder_state, &record->control_data,
                &record->event_ticks, &record->event_cursor);
            record->event_ticks = record->event_ticks + 1;
            unit_apply_control_block((uint32_t)record->unit_index, &record->control_data, -1);
            if (not_finished == 0) {
                record->flags = record->flags | _recorded_animation_flag_finished;
            } else {
                record->flags = record->flags & ~(uint16_t)_recorded_animation_flag_finished;
            }
        }
        record = (recorded_animation *)data_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x44aa90):

void recorded_animations_update(void)

{
  int *piVar1;
  uint *puVar2;
  byte *pbVar3;
  byte bVar4;
  int iVar5;
  int iVar6;
  char cVar7;
  short sVar8;
  int iVar9;
  int iVar10;
  short *psVar11;
  uint uVar12;
  short *psVar13;
  int iVar14;
  short sVar15;

  iVar9 = data_iterator_next();
  iVar14 = DAT_008603b0;
  do {
    if (iVar9 == 0) {
      return;
    }
    uVar12 = *(uint *)(iVar9 + 4);
    psVar13 = (short *)0x0;
    if (((uVar12 != 0xffffffff) && (sVar8 = (short)uVar12, -1 < sVar8)) &&
       (sVar8 < *(short *)(iVar14 + 0x20))) {
      psVar11 = (short *)((int)*(short *)(iVar14 + 0x22) * (int)sVar8 + *(int *)(iVar14 + 0x34));
      sVar8 = *psVar11;
      if ((sVar8 != 0) && ((sVar15 = (short)(uVar12 >> 0x10), sVar15 == 0 || (sVar8 == sVar15)))) {
        psVar13 = psVar11;
      }
    }
    if (((psVar13 == (short *)0x0) || ((1 << (*(byte *)((int)psVar13 + 3) & 0x1f) & 3U) == 0)) ||
       (*(int *)(psVar13 + 4) == 0)) {
LAB_0044ac8d:
      datum_delete();
    }
    else {
      if ((*(ushort *)(iVar9 + 10) & 1) != 0) {
        iVar5 = *(int *)(*(int *)(iVar14 + 0x34) + 8 + (uVar12 & 0xffff) * 0xc);
        uVar12 = *(uint *)(iVar5 + 0x204);
        if (((byte)*(ushort *)(iVar9 + 10) >> 2 & 1) == 0) {
          uVar12 = uVar12 & 0xffffffbf;
        }
        else {
          uVar12 = uVar12 | 0x40;
        }
        *(uint *)(iVar5 + 0x204) = uVar12;
        puVar2 = (uint *)(*(int *)(*(int *)(iVar14 + 0x34) + 8 +
                                  (*(uint *)(iVar9 + 4) & 0xffff) * 0xc) + 0x204);
        *puVar2 = *puVar2 & 0xf7ffffff;
        unit_refresh_targeting_flag_and_weapons(*(undefined4 *)(iVar9 + 4));
        iVar5 = *(int *)(iVar14 + 0x34);
        iVar10 = (*(uint *)(iVar9 + 4) & 0xffff) * 0xc;
        iVar6 = *(int *)(iVar5 + 8 + iVar10);
        *(byte *)(iVar5 + iVar10 + 2) = *(byte *)(iVar5 + 2 + iVar10) | 0x40;
        if ((*(int *)(iVar6 + 0x11c) == -1) && (*(short *)(iVar6 + 0x9c) == -1)) {
          iVar10 = *(int *)(iVar14 + 0x34) + iVar10;
          bVar4 = *(byte *)(iVar10 + 2);
          if ((bVar4 & 1) != 0) {
            *(byte *)(iVar10 + 2) = bVar4 & 0xfe;
          }
        }
        if ((((*(byte *)(iVar9 + 10) & 8) != 0) && (*(int *)(iVar9 + 4) != -1)) &&
           (cVar7 = hs_object_hierarchy_test(*(int *)(iVar9 + 4)), cVar7 == '\0')) {
          object_delete();
          iVar14 = DAT_008603b0;
        }
        if (((*(byte *)(iVar9 + 10) & 0x10) != 0) && (*(uint *)(iVar9 + 4) != 0xffffffff)) {
          iVar9 = *(int *)(*(int *)(iVar14 + 0x34) + 8 + (*(uint *)(iVar9 + 4) & 0xffff) * 0xc);
          object_get_position();
          pbVar3 = (byte *)(iVar9 + 0x4cc);
          *pbVar3 = *pbVar3 | 2;
        }
        goto LAB_0044ac8d;
      }
      *(short *)(iVar9 + 8) = *(short *)(iVar9 + 8) + -1;
      piVar1 = (int *)(iVar9 + 0xc);
      cVar7 = (**(code **)((&PTR_PTR_00686fe8)[*(short *)(iVar9 + 0x60)] + 4))
                        (iVar9 + 0x54,iVar9 + 0x14,piVar1,iVar9 + 0x10);
      *piVar1 = *piVar1 + 1;
      unit_apply_control_block(0xffffffff);
      iVar14 = DAT_008603b0;
      if (cVar7 == '\0') {
        *(byte *)(iVar9 + 10) = *(byte *)(iVar9 + 10) | 1;
      }
      else {
        *(byte *)(iVar9 + 10) = *(byte *)(iVar9 + 10) & 0xfe;
      }
    }
    iVar9 = data_iterator_next();
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
