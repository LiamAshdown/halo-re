// actor_refresh_combat_context  (Ghidra: actor_refresh_combat_context, already named)
// address 0x4297a0, size 1845 bytes
// name confidence: 0.5   rewrite confidence: 0.2
// evidence: types/ai.h actor.swarm(0x06)/unit_index(0x18)/actor_definition_tag(0x58)/
//   flying(0x99)/active_unit_index(0x158)/unknown_15e/unknown_15c/unknown_15d/unknown_160/
//   unknown_161/unknown_162/unknown_164..170/unknown_1b0/unknown_1b4/unknown_1b5/
//   facing/facing_unknown_180/facing_unknown_18c(0x174/0x180/0x18c)/unknown_18c..1c4/
//   danger_type(0x280)/danger_object_index(0x28c)/team(0x3e)/unknown_40/unknown_44/
//   unknown_48; swarm fields (component_count, unit_index[]/component_index[]),
//   swarm_component.position/marker_index. Given the size and the number of Unit- and
//   ActorType-tag-shaped raw offsets this function reaches into (none of which
//   types/tags.h or types/units.h currently name at the specific sub-fields used here), this
//   rewrite is deliberately kept close to the Ghidra decompilation rather than fully
//   re-derived, the same tradeoff src/ai/actor_squad_action_execute.c documents for a
//   function of comparable scope. Calls actor_fill_unit_position_context (0x4296c0, already
//   rewritten in this module, with its own low confidence), actor_reset_squad_link_for_type_change
//   (0x4290f0, already rewritten in this module), actor_get_actor_definition (0x40fa70),
//   vector2d_normalize_with_length (0x4018e0), vector3d_normalize_with_length (0x401990),
//   vector3d_cross_product (0x4052c0), object_get_position (0x4f6900),
//   object_get_node_local_transform (0x4f6080), and scenario_location_get_water_and_weather/unit_get_forward_vector_or_marker_normal, neither
//   established elsewhere in this repo.
// UNSURE: essentially every raw offset comment in this file marks a field this rewrite did
// not independently re-derive; see individual comments below. Preserved exactly as decompiled.
// reconciled: R04 0x006f1d20 uint8_t use_absolute_team_check -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>

extern data_array *actor_data;           // 0x00880360
extern data_array *object_data;          // 0x008603b0
extern data_array *swarm_data;           // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358
extern data_array *encounter_data;       // 0x008802c8
extern encounter_squad_state *encounter_squad_states; // 0x008802cc
extern tag_instance *tag_instances;      // 0x0087bc14

extern const real_vector3d *global_forward3d_pointer; // 0x00696718
extern const real_point3d *swarm_aggregate_seed;      // 0x006966f8, UNSURE name: a pointer to
                                      //   three floats, read only as the seed of the swarm
                                      //   aggregate position below
extern const real_vector3d *global_up3d_pointer;      // 0x00696720, UNSURE name
extern char ai_marker_name_b[];       // 0x00672034, UNSURE name/size
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern uint8_t team_relationship_flags; // 0x006b0b84, base of the 0x2d-dword team-relationship block

extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, UNSURE exact signature
extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name, object_marker *marker, uint32_t flags); // 0x4f6080
extern uint8_t scenario_location_get_water_and_weather(void *context, int32_t param); // 0x53ed60, UNSURE signature
extern void unit_get_forward_vector_or_marker_normal(void); // 0x569720, UNSURE signature (also called with no visible args elsewhere in this module)
extern void * actor_get_actor_definition(datum_index actor_index); // 0x40fa70, UNSURE signature (register args not traced here)
extern void actor_reset_squad_link_for_type_change(datum_index actor_index, datum_index encounter_index); // 0x4290f0
extern void actor_fill_unit_position_context(datum_index unit_index, void *out_context); // 0x4296c0

// Full per-tick recomputation of an actor's (or swarm's) combat context. For a swarm, this
// averages every component unit's position/marker into the swarm's own aggregate fields and
// then re-derives the position context for the swarm's lead unit. For a solo actor, this is
// the much larger path: it resolves the parent object (if any), the actor's per-unit
// position context, a lookahead transform, flying flag, current-target relationship bits
// (aim-target / threat / leader), a possible squad-link reassignment when the ActorType
// changes, a "nearby ally at a different position-cache index" scan across every object at
// the same location cluster, a pending-danger object scan, an aim-origin default from the
// unit's own tag data, and finally the facing/aim/eye-position vectors used everywhere else
// in this module.
// FIXED (register inputs, objdump + difftest): the original never reads EAX; actor_index arrive(s) on the stack (1 stack argument(s)).
// blam-cc: stack -> actor_index
void actor_refresh_combat_context(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_tag = (Actor *)(tag_instances[self->actor_definition_tag & 0xffff].data);

    if (self->swarm == 0) {
        object *unit_object = ((object_header *)object_data->data)[self->unit_index & 0xffff].data;
        object *parent_object = (unit_object->parent_object != (datum_index)k_datum_index_none)
                                     ? ((object_header *)object_data->data)[unit_object->parent_object & 0xffff].data
                                     : 0;

        actor_fill_unit_position_context(self->unit_index, (uint8_t *)self + 0x120);
        object_get_node_local_transform(self->unit_index, ai_marker_name_b,
                                        (object_marker *)&self->unknown_15d /* UNSURE, 108-byte scratch */, 1);
        self->unknown_15d = scenario_location_get_water_and_weather((uint8_t *)self + 0x144, 0); // UNSURE offset/signature
        self->flying = (uint8_t)(*(uint32_t *)actor_tag >> 0x15) & 1;

        if (parent_object == 0 || *(int16_t *)((uint8_t *)parent_object + 0xb4) != 1) { // UNSURE: parent object type
            self->active_unit_index = (datum_index)k_datum_index_none;
            self->unknown_15e = 0;
            self->order_committed = 0;
            self->unknown_161 = 0;
            if (self->unknown_40[0] != 0) {
                actor_reset_squad_link_for_type_change(actor_index, *(int16_t *)((uint8_t *)self + 0x48));
                self->unknown_40[0] = 0;
            }
        } else {
            Actor *parent_actor_tag = (Actor *)(tag_instances[*(uint16_t *)parent_object & 0xffff].data); // UNSURE
            self->unknown_161 = 0;
            self->unknown_162[0] = 0;
            self->unknown_15e = 0;
            self->active_unit_index = unit_object->parent_object;

            if (*(uint32_t *)((uint8_t *)parent_object + 0xc9 * 4) == self->unit_index) { // UNSURE offset
                self->unknown_15e = 1;
                {
                    uint32_t flags = *(uint32_t *)((uint8_t *)parent_actor_tag + 0x2f0); // UNSURE offset
                    if ((flags & 0x800) != 0) {
                        if ((flags & 0x1000) == 0) {
                            if ((flags & 0x2000) != 0) {
                                self->unknown_15e = ((~(flags >> 0xe)) & 1) | 2;
                            }
                        } else {
                            self->unknown_15e = 4;
                            self->flying = 1;
                        }
                    }
                }
            }
            if (*(uint32_t *)((uint8_t *)parent_object + 0xca * 4) == self->unit_index) { // UNSURE offset
                void *definition;
                self->unknown_161 = 1;
                // The original reads +0x14c out of actor_get_actor_definition's RETURN value,
                // not out of the tag data resolved at the top of the function. The two are very
                // likely the same pointer, but the call's result is what is used here.
                definition = actor_get_actor_definition(actor_index);
                self->unknown_162[0] = *(float *)((uint8_t *)definition + 0x14c) > 0.0f; // UNSURE offset
            }
            self->order_committed = self->unknown_15e < 2;

            if (*(int16_t *)((uint8_t *)parent_object + 0xcd * 4) != -1) { // UNSURE offset
                datum_index encounter_index = self->encounter_index;
                uint16_t encounter_slot = (uint16_t)encounter_index;

                if (encounter_slot != (uint16_t)*(int16_t *)((uint8_t *)parent_object + 0xcd * 4) ||
                    (*(int16_t *)((uint8_t *)parent_object + 0x336) != -1 &&
                     self->squad_index != *(int16_t *)((uint8_t *)parent_object + 0x336) &&
                     (encounter_data && encounter_squad_states[encounter_slot].squad_delay_ticks /* UNSURE: encounter+0x62 via squad_index? */ < 1))) {
                    if (self->unknown_40[0] == 0) {
                        *(datum_index *)&self->unknown_40[4] = encounter_index;
                        *(int16_t *)((uint8_t *)self + 0x48) = self->squad_index;
                        self->unknown_40[0] = 1;
                        if (encounter_index != (datum_index)k_datum_index_none) {
                            ((encounter *)encounter_data->data)[encounter_index & 0xffff].dirty = 1;
                        }
                    }
                    actor_reset_squad_link_for_type_change(actor_index, *(int16_t *)((uint8_t *)parent_object + 0x336));
                }
            }
        }

        self->unknown_1b4[1] = *(uint8_t *)((uint8_t *)unit_object + 0x28b) != 0; // offset 0x1b5
        self->unknown_1b4[0] = 0;
        self->unknown_1b0 = 0xffffffff;

        {
            // object+0x118 is first_child_object and object+0x114 is next_object: this is a
            // walk of the unit's attached-object list, not of a location cluster. (The
            // first rewrite read object.location_leaf_index at +0x98 here.)
            datum_index cursor = unit_object->first_child_object;
            while (cursor != (datum_index)k_datum_index_none) {
                object *candidate = ((object_header *)object_data->data)[cursor & 0xffff].data;
                if (candidate->type == 0) {
                    int16_t candidate_team = *(int16_t *)((uint8_t *)candidate + 0xb8); // UNSURE offset
                    int mismatch;
                    if (current_game_engine == 0) {
                        if (self->team >= 0 && self->team < 10 && candidate_team >= 0 && candidate_team < 10) {
                            int index = candidate_team + self->team * 10;
                            // The bitmap starts 0xa4 bytes into the block at 0x006b0b84; the
                            // first rewrite dropped that displacement.
                            mismatch = (((1 << (index & 0x1f)) &
                                         *(uint32_t *)((uint8_t *)&team_relationship_flags +
                                                       0xa4 + (index >> 5) * 4)) == 0);
                        } else {
                            // Ghidra falls straight through to the "set the flag" store when
                            // either team is outside 0..9, i.e. it counts as a mismatch.
                            mismatch = 1;
                        }
                    } else {
                        mismatch = self->team != candidate_team;
                    }
                    if (mismatch) {
                        self->unknown_1b4[0] = 1;
                    }
                } else if (candidate->type == 5 &&
                          (*(int8_t *)((uint8_t *)candidate + 0x22c) < 0 || // UNSURE offset
                           (self->danger_type == 2 && cursor == self->danger_object_index))) {
                    self->unknown_1b0 = cursor;
                }
                cursor = candidate->next_object; // object + 0x114
            }
        }

        self->unknown_15c = 0;
        self->unknown_164 = 0xffffffff;
        if (unit_object->type == 0 && self->active_unit_index == (datum_index)k_datum_index_none) {
            object *own_unit = ((object_header *)object_data->data)[self->unit_index & 0xffff].data;
            if (*(int8_t *)((uint8_t *)own_unit + 0x501) > 5) { // UNSURE offset
                self->unknown_15c = 1;
            }
            self->unknown_164 = *(int32_t *)((uint8_t *)own_unit + 0x4dc); // UNSURE offset
            *(int32_t *)((uint8_t *)self + 0x168) = *(int32_t *)((uint8_t *)own_unit + 0x4e0);
            *(int32_t *)((uint8_t *)self + 0x16c) = *(int32_t *)((uint8_t *)own_unit + 0x4e4);
            *(int32_t *)((uint8_t *)self + 0x170) = *(int32_t *)((uint8_t *)own_unit + 0x4e8);
        }

        unit_get_forward_vector_or_marker_normal();

        if (self->flying == 0) {
            // UNSURE: which vector is normalized -- the argument is register-passed and Ghidra
            // attributes none, so actor.facing (0x174, the only vector written by both arms of
            // the test below) is the reconstruction. What is NOT unsure is that the test is on
            // the RETURN value (the length) and not on a component of the vector: an earlier
            // draft tested dir.i, which inverts the branch whenever i is negative.
            real length = vector2d_normalize_with_length((real_vector2d *)&self->facing);
            if (length <= 0.0f) {
                self->facing = *global_forward3d_pointer;
            } else {
                self->facing.k = 0.0f;
            }
        }

        if (self->unknown_161 == 0) {
            self->facing_unknown_180 = *(real_vector3d *)((uint8_t *)unit_object + 0x23c); // UNSURE offset
        } else {
            object *active_unit = ((object_header *)object_data->data)[self->active_unit_index & 0xffff].data;
            Actor *active_tag = (Actor *)(tag_instances[*(uint16_t *)active_unit & 0xffff].data); // UNSURE
            if ((*(uint32_t *)((uint8_t *)active_tag + 0x2f0) & 0x100) == 0) {
                self->facing_unknown_180 = *(real_vector3d *)((uint8_t *)active_unit + 0x23c); // UNSURE offset
            } else {
                unit_get_forward_vector_or_marker_normal();
            }
        }

        self->facing_unknown_18c = *(real_vector3d *)((uint8_t *)unit_object + 0x260); // UNSURE offset

        {
            real_vector3d tmp;
            vector3d_cross_product(&tmp, global_up3d_pointer, &self->facing_unknown_18c);
            vector3d_normalize_with_length(&tmp);
            vector3d_cross_product(&tmp, &self->facing_unknown_18c, &self->facing_unknown_18c); // UNSURE args
        }

        *(real_point3d *)((uint8_t *)self + 0x1b8) = *(real_point3d *)((uint8_t *)unit_object + 0xe0);
        *(float *)((uint8_t *)self + 0x1c0) = *(float *)((uint8_t *)unit_object + 0xf8);
        *(float *)((uint8_t *)self + 0x1c4) = *(float *)((uint8_t *)unit_object + 0xf4);
    } else {
        swarm *s = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
        int16_t i;

        // The original seeds all THREE aggregate floats (swarm +0x0c/+0x10/+0x14) from the
        // three floats at 0x006966f8; an earlier draft of this file dropped the +0x0c one and
        // started the sum at +0x10, which shifted the whole centroid by one component.
        s->aggregate_position = *swarm_aggregate_seed;

        for (i = 0; i < s->component_count; i++) {
            swarm_component *component = &((swarm_component *)swarm_component_data->data)[s->component_index[i] & 0xffff];
            object *unit_object = ((object_header *)object_data->data)[s->unit_index[i] & 0xffff].data;
            datum_index marker = (unit_object->type == 0) ? *(datum_index *)((uint8_t *)unit_object + 0x4d8)
                                                           : (datum_index)k_datum_index_none;

            object_get_position(&component->position, s->unit_index[i]);
            component->marker_index = marker;

            s->aggregate_position.x = s->aggregate_position.x + component->position.x;
            s->aggregate_position.y = s->aggregate_position.y + component->position.y;
            s->aggregate_position.z = s->aggregate_position.z + component->position.z;
        }

        if (s->component_count > 0) {
            float inv = 1.0f / (float)s->component_count;
            s->aggregate_position.x = s->aggregate_position.x * inv;
            s->aggregate_position.y = s->aggregate_position.y * inv;
            s->aggregate_position.z = s->aggregate_position.z * inv;
        }

        memset((uint8_t *)self + 0x120, 0, 0x2a * sizeof(uint32_t));
        self->active_unit_index = (datum_index)k_datum_index_none;
        self->unknown_164 = 0xffffffff;

        if (self->cluster_unit_index != (datum_index)k_datum_index_none) {
            actor_fill_unit_position_context(self->cluster_unit_index, (uint8_t *)self + 0x120);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4297a0):

void actor_refresh_combat_context(uint param_1)

{
  float *pfVar1;
  short sVar2;
  uint *puVar3;
  int iVar4;
  uint uVar5;
  float fVar6;
  undefined *puVar7;
  undefined1 uVar8;
  short sVar9;
  int iVar10;
  uint uVar11;
  uint *puVar12;
  int iVar13;
  int iVar14;
  undefined4 *puVar15;
  char cVar16;
  float10 fVar17;
  undefined4 local_7c;
  undefined1 local_6c [108];

  puVar7 = PTR_DAT_006966f8;
  iVar13 = (param_1 & 0xffff) * 0x724;
  iVar14 = iVar13 + *(int *)(DAT_00880360 + 0x34);
  puVar3 = *(uint **)((*(uint *)(iVar13 + 0x58 + *(int *)(DAT_00880360 + 0x34)) & 0xffff) * 0x20 +
                      0x14 + DAT_0087bc14);
  if (*(char *)(iVar14 + 6) == '\0') {
    iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar14 + 0x18) & 0xffff) * 0xc)
    ;
    uVar5 = *(uint *)(iVar13 + 0x11c);
    if (uVar5 == 0xffffffff) {
      puVar12 = (uint *)0x0;
    }
    else {
      puVar12 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc);
    }
    FUN_004296c0(iVar14 + 0x120);
    object_get_node_local_transform(*(undefined4 *)(iVar14 + 0x18),&DAT_00672034,local_6c,1);
    uVar8 = FUN_0053ed60(iVar14 + 0x144,0);
    *(undefined1 *)(iVar14 + 0x15d) = uVar8;
    *(byte *)(iVar14 + 0x99) = (byte)(*puVar3 >> 0x15) & 1;
    if ((puVar12 == (uint *)0x0) || ((short)puVar12[0x2d] != 1)) {
      *(undefined4 *)(iVar14 + 0x158) = 0xffffffff;
      *(undefined2 *)(iVar14 + 0x15e) = 0;
      *(undefined1 *)(iVar14 + 0x160) = 0;
      *(undefined1 *)(iVar14 + 0x161) = 0;
      if (*(char *)(iVar14 + 0x40) != '\0') {
        FUN_004290f0(*(undefined2 *)(iVar14 + 0x48));
        *(undefined1 *)(iVar14 + 0x40) = 0;
      }
    }
    else {
      iVar10 = *(int *)((*puVar12 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      *(undefined1 *)(iVar14 + 0x161) = 0;
      *(undefined1 *)(iVar14 + 0x162) = 0;
      *(undefined2 *)(iVar14 + 0x15e) = 0;
      *(uint *)(iVar14 + 0x158) = uVar5;
      if (puVar12[0xc9] == *(uint *)(iVar14 + 0x18)) {
        *(undefined2 *)(iVar14 + 0x15e) = 1;
        uVar5 = *(uint *)(iVar10 + 0x2f0);
        if ((uVar5 & 0x800) != 0) {
          if ((uVar5 & 0x1000) == 0) {
            if ((uVar5 & 0x2000) != 0) {
              *(ushort *)(iVar14 + 0x15e) = (byte)~(byte)(uVar5 >> 0xe) & 1 | 2;
            }
          }
          else {
            *(undefined2 *)(iVar14 + 0x15e) = 4;
            *(undefined1 *)(iVar14 + 0x99) = 1;
          }
        }
      }
      if (puVar12[0xca] == *(uint *)(iVar14 + 0x18)) {
        *(undefined1 *)(iVar14 + 0x161) = 1;
        iVar10 = actor_get_actor_definition();
        *(bool *)(iVar14 + 0x162) = 0.0 < *(float *)(iVar10 + 0x14c);
      }
      *(bool *)(iVar14 + 0x160) = *(short *)(iVar14 + 0x15e) < 2;
      if ((short)puVar12[0xcd] != -1) {
        uVar5 = *(uint *)(iVar14 + 0x34);
        uVar11 = uVar5 & 0xffff;
        if ((uVar11 != (int)(short)puVar12[0xcd]) ||
           (((*(short *)((int)puVar12 + 0x336) != -1 &&
             (*(short *)(iVar14 + 0x3a) != *(short *)((int)puVar12 + 0x336))) &&
            ((iVar10 = uVar11 * 0x6c + *(int *)(DAT_008802c8 + 0x34), *(short *)(iVar10 + 0x62) < 1
             || ((sVar9 = *(short *)(iVar10 + 4),
                 *(char *)((short)(*(short *)(iVar14 + 0x3a) + sVar9) * 0x20 + 0x10 + DAT_008802cc)
                 == '\0' ||
                 (*(char *)((short)(sVar9 + *(short *)((int)puVar12 + 0x336)) * 0x20 + 0x10 +
                           DAT_008802cc) == '\0')))))))) {
          if (*(char *)(iVar14 + 0x40) == '\0') {
            *(uint *)(iVar14 + 0x44) = uVar5;
            *(undefined2 *)(iVar14 + 0x48) = *(undefined2 *)(iVar14 + 0x3a);
            *(undefined1 *)(iVar14 + 0x40) = 1;
            if (uVar5 != 0xffffffff) {
              *(undefined1 *)(uVar11 * 0x6c + 0x1e + *(int *)(DAT_008802c8 + 0x34)) = 1;
            }
          }
          FUN_004290f0(*(undefined2 *)((int)puVar12 + 0x336));
        }
      }
    }
    *(bool *)(iVar14 + 0x1b5) = *(char *)(iVar13 + 0x28b) != '\0';
    *(undefined1 *)(iVar14 + 0x1b4) = 0;
    *(undefined4 *)(iVar14 + 0x1b0) = 0xffffffff;
    uVar5 = *(uint *)(iVar13 + 0x118);
    while (uVar5 != 0xffffffff) {
      iVar10 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc);
      if (*(short *)(iVar10 + 0xb4) == 0) {
        sVar9 = *(short *)(iVar10 + 0xb8);
        sVar2 = *(short *)(iVar14 + 0x3e);
        if (DAT_006f1d20 == 0) {
          if ((((-1 < sVar2) && (sVar2 < 10)) && (-1 < sVar9)) && (sVar9 < 10)) {
            iVar4 = (int)sVar9 + sVar2 * 10;
            cVar16 = '\x01' - ((1 << ((byte)iVar4 & 0x1f) &
                               *(uint *)(DAT_006b0b84 + 0xa4 + (iVar4 >> 5) * 4)) != 0);
            goto LAB_00429ca3;
          }
        }
        else {
          cVar16 = sVar2 != sVar9;
LAB_00429ca3:
          if (cVar16 == '\0') goto LAB_00429cb0;
        }
        *(undefined1 *)(iVar14 + 0x1b4) = 1;
      }
      else {
LAB_00429cb0:
        if ((*(short *)(iVar10 + 0xb4) == 5) &&
           ((*(char *)(iVar10 + 0x22c) < '\0' ||
            ((*(short *)(iVar14 + 0x280) == 2 && (uVar5 == *(uint *)(iVar14 + 0x28c))))))) {
          *(uint *)(iVar14 + 0x1b0) = uVar5;
        }
      }
      uVar5 = *(uint *)(iVar10 + 0x114);
    }
    *(undefined1 *)(iVar14 + 0x15c) = 0;
    *(undefined4 *)(iVar14 + 0x164) = 0xffffffff;
    if ((*(short *)(iVar13 + 0xb4) == 0) && (*(int *)(iVar14 + 0x158) == -1)) {
      iVar10 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                       (*(uint *)(iVar14 + 0x18) & 0xffff) * 0xc);
      if ('\x05' < *(char *)(iVar10 + 0x501)) {
        *(undefined1 *)(iVar14 + 0x15c) = 1;
      }
      *(undefined4 *)(iVar14 + 0x164) = *(undefined4 *)(iVar10 + 0x4dc);
      *(undefined4 *)(iVar14 + 0x168) = *(undefined4 *)(iVar10 + 0x4e0);
      *(undefined4 *)(iVar14 + 0x16c) = *(undefined4 *)(iVar10 + 0x4e4);
      *(undefined4 *)(iVar14 + 0x170) = *(undefined4 *)(iVar10 + 0x4e8);
    }
    FUN_00569720();
    if (*(char *)(iVar14 + 0x99) == '\0') {
      fVar17 = (float10)vector2d_normalize_with_length();
      puVar7 = PTR_DAT_00696718;
      if (fVar17 <= (float10)0.0) {
        *(undefined4 *)(iVar14 + 0x174) = *(undefined4 *)PTR_DAT_00696718;
        *(undefined4 *)(iVar14 + 0x178) = *(undefined4 *)(puVar7 + 4);
        *(undefined4 *)(iVar14 + 0x17c) = *(undefined4 *)(puVar7 + 8);
      }
      else {
        *(undefined4 *)(iVar14 + 0x17c) = 0;
      }
    }
    if (*(char *)(iVar14 + 0x161) == '\0') {
      *(undefined4 *)(iVar14 + 0x180) = *(undefined4 *)(iVar13 + 0x23c);
      *(undefined4 *)(iVar14 + 0x184) = *(undefined4 *)(iVar13 + 0x240);
      *(undefined4 *)(iVar14 + 0x188) = *(undefined4 *)(iVar13 + 0x244);
    }
    else {
      puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                         (*(uint *)(iVar14 + 0x158) & 0xffff) * 0xc);
      if ((*(uint *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f0) & 0x100) ==
          0) {
        *(uint *)(iVar14 + 0x180) = puVar3[0x8f];
        *(uint *)(iVar14 + 0x184) = puVar3[0x90];
        *(uint *)(iVar14 + 0x188) = puVar3[0x91];
      }
      else {
        FUN_00569720();
      }
    }
    *(undefined4 *)(iVar14 + 0x18c) = *(undefined4 *)(iVar13 + 0x260);
    *(undefined4 *)(iVar14 + 400) = *(undefined4 *)(iVar13 + 0x264);
    *(undefined4 *)(iVar14 + 0x194) = *(undefined4 *)(iVar13 + 0x268);
    vector3d_cross_product(PTR_DAT_00696720);
    vector3d_normalize_with_length();
    vector3d_cross_product((undefined4 *)(iVar14 + 0x18c));
    *(undefined4 *)(iVar14 + 0x1b8) = *(undefined4 *)(iVar13 + 0xe0);
    *(undefined4 *)(iVar14 + 0x1bc) = *(undefined4 *)(iVar13 + 0xe4);
    *(undefined4 *)(iVar14 + 0x1c0) = *(undefined4 *)(iVar13 + 0xf8);
    *(undefined4 *)(iVar14 + 0x1c4) = *(undefined4 *)(iVar13 + 0xf4);
  }
  else {
    iVar13 = (*(uint *)(iVar14 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
    pfVar1 = (float *)(iVar13 + 0xc);
    *pfVar1 = *(float *)PTR_DAT_006966f8;
    *(undefined4 *)(iVar13 + 0x10) = *(undefined4 *)(puVar7 + 4);
    *(undefined4 *)(iVar13 + 0x14) = *(undefined4 *)(puVar7 + 8);
    sVar9 = 0;
    if (0 < *(short *)(iVar13 + 2)) {
      do {
        iVar10 = (*(uint *)(iVar13 + 0x58 + sVar9 * 4) & 0xffff) * 0x40 +
                 *(int *)(DAT_00880358 + 0x34);
        iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                        (*(uint *)(iVar13 + 0x18 + sVar9 * 4) & 0xffff) * 0xc);
        local_7c = 0xffffffff;
        if (*(short *)(iVar4 + 0xb4) == 0) {
          local_7c = *(undefined4 *)(iVar4 + 0x4d8);
        }
        object_get_position();
        *(undefined4 *)(iVar10 + 0x10) = local_7c;
        sVar9 = sVar9 + 1;
        *pfVar1 = *(float *)(iVar10 + 4) + *pfVar1;
        *(float *)(iVar13 + 0x10) = *(float *)(iVar10 + 8) + *(float *)(iVar13 + 0x10);
        *(float *)(iVar13 + 0x14) = *(float *)(iVar10 + 0xc) + *(float *)(iVar13 + 0x14);
      } while (sVar9 < *(short *)(iVar13 + 2));
    }
    if (0 < *(short *)(iVar13 + 2)) {
      fVar6 = 1.0 / (float)(int)*(short *)(iVar13 + 2);
      *pfVar1 = fVar6 * *pfVar1;
      *(float *)(iVar13 + 0x10) = fVar6 * *(float *)(iVar13 + 0x10);
      *(float *)(iVar13 + 0x14) = fVar6 * *(float *)(iVar13 + 0x14);
    }
    puVar15 = (undefined4 *)(iVar14 + 0x120);
    for (iVar13 = 0x2a; iVar13 != 0; iVar13 = iVar13 + -1) {
      *puVar15 = 0;
      puVar15 = puVar15 + 1;
    }
    *(undefined4 *)(iVar14 + 0x158) = 0xffffffff;
    *(undefined4 *)(iVar14 + 0x164) = 0xffffffff;
    if (*(int *)(iVar14 + 0x24) != -1) {
      FUN_004296c0((undefined4 *)(iVar14 + 0x120));
      return;
    }
  }
  return;
}
#endif
