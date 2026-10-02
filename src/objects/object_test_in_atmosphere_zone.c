// object_test_in_atmosphere_zone  (Ghidra: FUN_004f76e0; renamed, Blam-style, not previously
// named)
// address 0x4f76e0, size 607 bytes
// name confidence: 0.3 (matches functions.md's summary: "Tests whether an object currently
//   lies within one of the map's atmospheric/weather effect zones")
// rewrite confidence: 0.85 (VERIFIED against objdump; player iteration FIXED) (raised from 0.25 by the phase-4 review pass: the leaf/cluster lookup, the PVS base offset and the out-block type were all corrected against the disassembly) (single caller, heavy foreign-module dependency: cluster PVS bits,
//   an unnamed BSP/weather-zone iterator (datum_next), and a trig-heavy cone/angle test
//   against per-zone plane data at an unidentified stride-0x200 array. Preserved close to the
//   original pointer arithmetic rather than fully re-derived; see UNSURE notes below)
// evidence: types/objects.h object_header (flags 0x02 active bit), object (flags 0x10 with bit
//   0x800/_object_needs_cluster_update_bit and bit 0x200000/_object_outside_map_bit,
//   bounding_center 0x0a0, bounding_radius 0x0ac); global 0x008603b0 object_data, global
//   0x0087a478 local_player_globals (same global objects_update.c and
//   object_set_cluster_and_parent.c use); callees object_get_root_parent_placement (0x4f5f70,
//   established: EAX -> object_index, ESI -> out_pair), vector3d_normalize_with_length
//   (0x401990), object_get_node_local_transform (0x4f6080, OUTSIDE this batch -- see the
//   UNSURE note in object_solve_two_bone_ik_to_marker.c about its true calling convention).
// register convention: object index in EAX. Confirmed against objdump -d -M intel bin/halo.exe:
//   0x4f76ee mov esi,eax at entry, then esi is masked as the object index immediately.
//   // blam-cc: EAX -> object_index
// UNSURE: DAT_0087a480 (here walked as a stride-0x200 array via *(int*)(DAT_0087a480+0x34) as
//   its data_array->data base) is documented elsewhere in this codebase as the players module's
//   `player_data`; whether this call site is genuinely reusing that array for an unrelated
//   per-zone record, or the global address coincidence hides a second, distinct structure, is
//   not resolved here. A separate name is used below rather than importing the `player_data`
//   assumption. UNSURE: datum_next (an apparent stateful zone/leaf iterator) and FUN_004f6080
//   (object_get_node_local_transform, called here with a fixed marker-name constant
//   DAT_0066bfa0) are both declared with the plain argument lists Ghidra itself shows, which is
//   almost certainly incomplete for datum_next (shown with empty parens both times it is
//   called, yet its result clearly threads iterator state).

// FIXED (0x4f781b: [esp+0xa8] = marker +0x60): the marker position is the WORLD position
//   node_transform.position, not the node-relative transform at +0x2c.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern uint8_t *local_player_globals; // 0x0087a478, same declaration as objects_update.c and
    // object_set_cluster_and_parent.c: the PVS dwords start at +0x18, not at the base
extern data_array *player_data; // 0x0087a480, UNSURE: see file header
extern char ai_marker_name_a[]; // FIXED: an array ("head"); its address is the name -- // 0x0066bfa0, UNSURE: a fixed marker-name string

extern int16_t object_get_root_parent_placement(uint32_t object_index,
    object_placement_cursor *out_cursor); // 0x4f5f70, blam-cc: EAX -> object_index, ESI -> out_cursor
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, v in ECX
extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, DX, EDI
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
    object_marker *marker, uint32_t flags); // 0x4f6080, OUTSIDE this batch, UNSURE: see file header
extern double atan2(double y, double x); // x87 FPATAN
extern double cos(double x); // x87 FCOS

uint8_t object_test_in_atmosphere_zone(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
    object *obj = header->data;
    uint8_t result = 0;

    if (((header->flags & _object_header_active_bit) != 0) &&
        ((obj->flags & _object_needs_cluster_update_bit) != 0) &&
        ((obj->flags & _object_outside_map_bit) == 0)) {
        // PHASE-4 REVIEW: this block previously declared the out block as a single
        // `int32_t *pair` and passed `&pair`, so object_get_root_parent_placement's second
        // output wrote past that local and `pair[1]` read the wrong global. It is now the
        // two-dword object_placement_cursor the callee actually fills.
        object_placement_cursor cursor;
        int16_t placement = object_get_root_parent_placement(object_index, &cursor);

        if (placement != -1) {
            uint32_t ref = (uint32_t)(uint16_t)placement;
            uint32_t ref_index = cursor.next_reference;
            // cluster_globals[2] is the third of the adjacent globals, read as the data_array
            // of object_cluster_reference records -- the same slot object_get_root_parent_
            // placement itself indexes (see its UNSURE note about that slot's identity).
            data_array *references = (data_array *)cursor.cluster_globals[2];

            // Walk the reference chain until its cluster's PVS bit is set (visible) or the
            // chain runs out. PHASE-4 REVIEW: the PVS dwords begin at local_player_globals
            // + 0x18 (0x4f7715: `mov eax,[eax*4 + 0x18 + DAT_0087a478]`); the earlier rewrite
            // indexed from the base and read six dwords too early.
            while ((*(uint32_t *)(local_player_globals + 0x18 + ((int16_t)ref >> 5) * 4) &
                    (1u << ((uint8_t)ref & 0x1f))) == 0) {
                if (ref_index == 0xffffffff) {
                    ref = 0xffffffff;
                } else {
                    // PHASE-4 REVIEW: the original dereferences the global AND THEN takes
                    // data_array.data at +0x34 before striding; the +0x34 step was missing.
                    object_cluster_reference *node =
                        (object_cluster_reference *)references->data + (ref_index & 0xffff);
                    ref_index = node->next_reference;
                    ref = node->object_index;
                }
                if ((int16_t)ref == -1) {
                    return 0;
                }
            }

            if ((int16_t)ref != -1) {
                float search_radius = obj->bounding_radius;
                // FIXED (objdump 0x4f77b9..0x4f77cc / 0x4f790f..0x4f7917): walk the PLAYER array (0x87a480) with
                //   datum_next(DX = previous, EDI = the array); the draft passed nothing.
                uint32_t zone_index = datum_next(-1, player_data);

                while (zone_index != 0xffffffff) {
                    uint8_t *zone_table = *(uint8_t **)((uint8_t *)player_data + 0x34);
                    int32_t zone_offset = (int32_t)(zone_index & 0xffff) * 0x200;
                    int32_t zone_cluster_head = *(int32_t *)(zone_table + zone_offset + 0x34);

                    if (zone_cluster_head != -1) {
                        object_marker marker;
                        float dx, dy, dz;

                        object_get_node_local_transform(zone_cluster_head, ai_marker_name_a, &marker, 1);
                        dx = obj->bounding_center.x - marker.node_transform.position.x;
                        dy = obj->bounding_center.y - marker.node_transform.position.y;
                        dz = obj->bounding_center.z - marker.node_transform.position.z;

                        if (search_radius * search_radius <= dx * dx + dy * dy + dz * dz) {
                            // UNSURE: object+0x230..0x238 is past the common object header
                            // (0x1f4), inside the unit/vehicle extension this module does not
                            // otherwise define; preserved as raw floats rather than a named
                            // field, matching *(float *)(iVar4+0x230) in the original.
                            uint8_t *extended = (uint8_t *)((object_header *)object_data->data)[
                                *(uint16_t *)(zone_table + zone_offset + 0x34) & 0xffff].data;
                            real_vector3d delta;
                            float length;
                            double angle;
                            double c;

                            delta.i = dx;
                            delta.j = dy;
                            delta.k = dz;
                            length = vector3d_normalize_with_length(&delta);

                            angle = atan2((double)search_radius, (double)length);
                            // UNSURE: fpatan's operand order (radius, length) preserved from the
                            // Ghidra decompile's own fpatan(fVar2, Var12) shape.
                            c = cos(angle + 0.7853982);

                            if (delta.i * *(float *)(extended + 0x230) +
                                delta.j * *(float *)(extended + 0x234) +
                                delta.k * *(float *)(extended + 0x238) <= c) {
                                zone_index = datum_next((int16_t)zone_index, player_data);
                                continue;
                            }
                        }
                    } else {
                        zone_index = datum_next((int16_t)zone_index, player_data);
                        continue;
                    }

                    result = 1;
                    break;
                }
            }
        }
    }

    return result;
}

#if 0
Original Ghidra decompilation (0x4f76e0):

undefined1 FUN_004f76e0(void)

{
  float fVar1;
  float fVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  uint in_EAX;
  undefined4 uVar9;
  uint uVar10;
  int iVar11;
  unkbyte10 Var12;
  float10 fVar13;
  undefined1 local_a1;
  int local_94;
  uint local_90;
  undefined1 local_78 [96];
  float local_18;
  float local_14;
  float local_10;

  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  local_a1 = 0;
  if (((((*(byte *)(*(int *)(DAT_008603b0 + 0x34) + (in_EAX & 0xffff) * 0xc + 2) & 1) != 0) &&
       ((*(uint *)(iVar3 + 0x10) & 0x800) != 0)) && ((*(uint *)(iVar3 + 0x10) & 0x200000) == 0)) &&
     (uVar9 = FUN_004f5f70(), (short)uVar9 != -1)) {
    while ((*(uint *)(DAT_0087a478 + 0x18 + ((int)(short)uVar9 >> 5) * 4) &
           1 << ((byte)uVar9 & 0x1f)) == 0) {
      if (local_90 == 0xffffffff) {
        uVar9 = 0xffffffff;
      }
      else {
        uVar10 = local_90 & 0xffff;
        iVar4 = *(int *)(*(int *)(local_94 + 8) + 0x34);
        local_90 = *(uint *)(iVar4 + 8 + uVar10 * 0xc);
        uVar9 = *(undefined4 *)(iVar4 + uVar10 * 0xc + 4);
      }
      if ((short)uVar9 == -1) {
        return 0;
      }
    }
    if ((short)uVar9 != -1) {
      fVar1 = *(float *)(iVar3 + 0xac);
      uVar10 = FUN_004d0630();
      if (uVar10 != 0xffffffff) {
        while( true ) {
          iVar4 = *(int *)(DAT_0087a480 + 0x34);
          iVar11 = (uVar10 & 0xffff) * 0x200;
          iVar5 = *(int *)(iVar11 + 0x34 + iVar4);
          if (iVar5 != -1) break;
LAB_004f790f:
          uVar10 = FUN_004d0630();
          if (uVar10 == 0xffffffff) {
            return 0;
          }
        }
        FUN_004f6080(iVar5,&DAT_0066bfa0,local_78,1);
        fVar2 = *(float *)(iVar3 + 0xa0) - local_18;
        fVar7 = *(float *)(iVar3 + 0xa4) - local_14;
        fVar6 = *(float *)(iVar3 + 0xa8) - local_10;
        if (fVar1 * fVar1 <= fVar2 * fVar2 + fVar7 * fVar7 + fVar6 * fVar6) {
          fVar6 = *(float *)(iVar3 + 0xa0) - local_18;
          iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                          (*(uint *)(iVar11 + iVar4 + 0x34) & 0xffff) * 0xc);
          fVar7 = *(float *)(iVar3 + 0xa4) - local_14;
          fVar8 = *(float *)(iVar3 + 0xa8) - local_10;
          fVar2 = *(float *)(iVar3 + 0xac);
          Var12 = vector3d_normalize_with_length();
          fVar13 = (float10)fpatan((float10)fVar2,Var12);
          fVar13 = (float10)fcos(fVar13 + (float10)0.7853982);
          if ((float10)fVar6 * (float10)*(float *)(iVar4 + 0x230) +
              (float10)fVar7 * (float10)*(float *)(iVar4 + 0x234) +
              (float10)fVar8 * (float10)*(float *)(iVar4 + 0x238) <= fVar13) goto LAB_004f790f;
        }
        local_a1 = 1;
      }
    }
  }
  return local_a1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
