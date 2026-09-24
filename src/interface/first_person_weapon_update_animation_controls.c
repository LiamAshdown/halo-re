// first_person_weapon_update_animation_controls  (Ghidra: already named)
// address 0x493740, size 1299 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/interface.h first_person_weapon_interface names this address directly
// ("first_person_weapon_update_animation_controls @0x493740"), and its animation_control
// (+0x8c) / previous_pose (+0x88c) fields are read/written here exactly as documented;
// out/phase4/interface_functions.md "Computes the local player's aim direction and builds the
// first-person weapon's animation control block (grip/trigger/lean values) each frame, feeding
// it to the animation-control system.".
// register convention: local_player_index in AX. // blam-cc: AX -> local_player_index
// Review pass (phase 4): rebuilt from the disassembly (0x493740..0x493c52). The first rewrite
// read the wrong tag and dropped most register arguments.
//  - weapon tag +0x468 is Weapon.first_person_model.tag_id (a model tag, handed to 0x4d7610 in
//    ESI) and +0x478 is first_person_animations.tag_id (ModelAnimations). The count/pointer pair
//    at +0x48/+0x4c is ModelAnimations.first_person_weapons; its element +0x10/+0x14 is the
//    int16 animation list (entry 4 overlays, entry 17 ammunition). +0x78 is animations.pointer
//    (stride 0xb4, frame_count at +0x22), +0x68 is nodes.count.
//  - Each animation-control helper takes the ModelAnimationsAnimation in a register: 0x4d4a80
//    (EDI, EAX = 0), 0x4d4f90 (ESI), 0x4d51a0 and 0x4d57d0 (EDI). 0x4d69e0 takes the control
//    block in EAX and the node count in CX. 0x4d6880 takes the animation graph in EAX and the
//    camera position (0x7c3114) in ECX.
//  - The ammunition frame is rounds_loaded (weapon object +0x2b8). For a needler (weapon_type
//    2 at +0x4e2) while local player 0 is in state 0xd or 0xe, and at least 0x2c ticks into the
//    magazine state (+0x2b4 total minus +0x2b2 remaining), the frame runs ahead toward
//    min(rounds_unloaded, magazines[0].rounds_loaded_maximum) at 0.2 per tick, capped at 1.
//    The state test reads first_person_weapon_interfaces[0], not the indexed entry.
//  - When the weapon has no first_person_weapons block, only the world transforms run; when
//    there is no weapon at all, nothing after the aim bookkeeping runs.
// UNSURE: the 0x4d4xxx animation helpers belong to the animation module and are not rewritten;
// their meanings (base frame, frame overlay, weighted overlay channel, blend) are inferred from
// the arguments alone. 0x7c312c is taken to be the camera up vector.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98
extern data_array *object_data; // 0x008603b0, "objects"
extern tag_instance *tag_instances; // 0x0087bc14
extern real_point3d camera_position;  // 0x007c3114
extern real_vector3d camera_forward;  // 0x007c3120
extern real_vector3d camera_up;       // 0x007c312c, UNSURE

extern double fpatan(double y, double x); // FPATAN: atan2(ST1, ST0)
extern double sqrt(double x);             // FSQRT
extern int32_t __ftol(double x);          // 0x6391b4
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0; blam-cc: ECX -> object_index
extern void model_nodes_get_default_transforms(void *model, void *animation_control); // 0x4d7610; blam-cc: ESI -> model
extern void animation_get_frame_orientations(int32_t zero, ModelAnimationsAnimation *animation, int32_t frame,
                         void *animation_control); // 0x4d4a80; blam-cc: EAX -> zero, EDI -> animation
extern void animation_overlay_frame_orientations(ModelAnimationsAnimation *animation, int32_t frame,
                         void *animation_control); // 0x4d4f90; blam-cc: ESI -> animation
extern void animation_overlay_interpolated_frame_orientations_weighted(ModelAnimationsAnimation *animation, float frame, float weight,
                         void *animation_control); // 0x4d57d0; blam-cc: EDI -> animation
extern void animation_overlay_frame_orientations_weighted(ModelAnimationsAnimation *animation, int32_t channel, float weight,
                         void *animation_control); // 0x4d51a0; blam-cc: EDI -> animation
extern void model_nodes_blend_transforms(void *animation_control, int16_t node_count,
                                         void *previous_pose, int32_t blend_start,
                                         int32_t blend_end); // 0x4d69e0; blam-cc: EAX -> animation_control, CX -> node_count
extern void animation_graph_nodes_build_matrices(datum_index animation_graph,
                                                   real_point3d *position, void *node_matrices,
                                                   void *animation_control, real_vector3d *forward,
                                                   real_vector3d *up); // 0x4d6880; blam-cc: EAX -> animation_graph, ECX -> position

#define FP_FLOAT(fp, offset) (*(float *)((uint8_t *)(fp) + (offset)))

static void seed_aim(first_person_weapon_interface *fp)
{
    FP_FLOAT(fp, 0x60) = (float)fpatan(camera_forward.j, camera_forward.i);
    FP_FLOAT(fp, 0x64) = (float)fpatan(camera_forward.k,
                                       sqrt(camera_forward.j * camera_forward.j +
                                            camera_forward.i * camera_forward.i));
    *(real_point3d *)((uint8_t *)fp + 0x70) = camera_position;
}

static void overlay_channel(ModelAnimationsAnimation *animation, float value, int32_t positive,
                            int32_t negative, void *animation_control)
{
    if (value > 0.0f) {
        animation_overlay_frame_orientations_weighted(animation, positive, value, animation_control);
    } else if (value < 0.0f) {
        animation_overlay_frame_orientations_weighted(animation, negative, -value, animation_control);
    }
}

void first_person_weapon_update_animation_controls(int16_t local_player_index)
{
    first_person_weapon_interface *fp = &first_person_weapon_interfaces[local_player_index];
    uint8_t *fp_raw = (uint8_t *)fp;

    // 0x60/0x64 aim yaw/pitch, 0x68/0x6c previous, 0x70 position, 0x7c previous position,
    // 0x54 forward, 0x50 "aim seeded" flag
    if (fp->unknown_30[0x20] == 0) {
        seed_aim(fp);
    }
    FP_FLOAT(fp, 0x68) = FP_FLOAT(fp, 0x60);
    FP_FLOAT(fp, 0x6c) = FP_FLOAT(fp, 0x64);
    *(real_point3d *)(fp_raw + 0x7c) = *(real_point3d *)(fp_raw + 0x70);
    seed_aim(fp);
    *(real_vector3d *)(fp_raw + 0x54) = camera_forward;
    fp->unknown_30[0x20] = 1;

    if (fp->weapon_index != (datum_index)-1 && object_try_and_get(fp->weapon_index, 4) == 0) {
        fp->weapon_index = (datum_index)-1;
    }
    if (fp->weapon_index == (datum_index)-1) {
        return;
    }

    {
        object *weapon_obj = *(object **)((char *)object_data->data + 8 +
                                          (fp->weapon_index & 0xffff) * 0xc);
        Weapon *weapon_tag = (Weapon *)tag_instances[weapon_obj->definition_tag & 0xffff].data;
        void *model = tag_instances[weapon_tag->first_person_model.tag_id.index].data;
        ModelAnimations *animations =
            (ModelAnimations *)tag_instances[weapon_tag->first_person_animations.tag_id.index].data;
        ModelAnimationsAnimation *animation_block;
        ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *list;
        int16_t *list_entries;
        void *animation_control = fp_raw + 0x8c;
        int16_t index;

        if (animations->first_person_weapons.count != 0 &&
            (list = (ModelAnimationsAnimationGraphFirstPersonWeaponAnimations *)
                 animations->first_person_weapons.pointer) != 0) {
            animation_block = (ModelAnimationsAnimation *)animations->animations.pointer;
            list_entries = (int16_t *)list->animations.pointer;

            if (fp->unknown_16 != -1) {
                animation_get_frame_orientations(0, &animation_block[fp->unknown_16],
                             (uint16_t)*(int16_t *)fp->unknown_18, animation_control);
            } else {
                model_nodes_get_default_transforms(model, animation_control);
            }

            // ammunition counter (list entry 17)
            if ((int32_t)list->animations.count > 0x11 && (index = list_entries[0x11]) != -1) {
                ModelAnimationsAnimation *ammunition = &animation_block[index];
                int16_t *magazine = (int16_t *)((uint8_t *)weapon_obj + 0x2b0); // magazines[0]

                if (weapon_tag->weapon_type == 2 &&
                    (first_person_weapon_interfaces[0].state == 0xd ||
                     first_person_weapon_interfaces[0].state == 0xe)) {
                    int16_t elapsed = (int16_t)(magazine[2] - magazine[1]);
                    int32_t frame = (uint16_t)magazine[4];            // rounds_loaded

                    if (elapsed >= 0x2c) {
                        WeaponMagazine *magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer;
                        float fraction = (float)(elapsed - 0x2c) * 0.2f;
                        int16_t target;
                        if (fraction > 1.0f) {
                            fraction = 1.0f;
                        }
                        target = magazine[3];                          // rounds_unloaded
                        if (target > (int16_t)magazine_tag->rounds_loaded_maximum) {
                            target = magazine_tag->rounds_loaded_maximum;
                        }
                        frame += __ftol((float)(target - (int16_t)frame) * fraction);
                    }
                    animation_overlay_frame_orientations(ammunition, frame, animation_control);
                } else if (magazine[4] < (int16_t)ammunition->frame_count) {
                    animation_overlay_frame_orientations(ammunition, (uint16_t)magazine[4], animation_control);
                }
            }

            if (fp->unknown_1a != -1) {
                animation_overlay_frame_orientations(&animation_block[fp->unknown_1a],
                             (uint16_t)*(int16_t *)fp->unknown_1c, animation_control);
            }
            if (fp->unknown_20 != -1) {
                animation_overlay_interpolated_frame_orientations_weighted(&animation_block[fp->unknown_20], FP_FLOAT(fp, 0x24),
                             *(float *)((uint8_t *)weapon_obj + 0x244) + 0.5f, animation_control);
            }

            // sway overlays (list entry 4), nine weighted channels
            if ((int32_t)list->animations.count > 4 && (index = list_entries[4]) != -1 &&
                (int16_t)animation_block[index].frame_count >= 9) {
                ModelAnimationsAnimation *overlays = &animation_block[index];
                overlay_channel(overlays, FP_FLOAT(fp, 0x30), 0, 1, animation_control);
                overlay_channel(overlays, FP_FLOAT(fp, 0x34), 3, 2, animation_control);
                overlay_channel(overlays, FP_FLOAT(fp, 0x40), 4, 5, animation_control);
                overlay_channel(overlays, FP_FLOAT(fp, 0x44), 7, 6, animation_control);
                if (fp->unknown_28 > 0.0f) {
                    animation_overlay_frame_orientations_weighted(overlays, 8, fp->unknown_28, animation_control);
                }
            }

            if (fp->blend_end > 0) {
                model_nodes_blend_transforms(animation_control, (int16_t)animations->nodes.count,
                                             fp_raw + 0x88c, (uint16_t)fp->blend_start,
                                             (uint16_t)fp->blend_end);
            }
        }

        animation_graph_nodes_build_matrices(
            *(datum_index *)&weapon_tag->first_person_animations.tag_id, &camera_position,
            fp_raw + 0x108c, fp_raw + 0x8c, &camera_forward, &camera_up);
    }
}

#if 0
Original Ghidra decompilation (0x493740):

void first_person_weapon_update_animation_controls(void)

{
  int iVar1;
  short sVar2;
  uint *puVar3;
  int iVar4;
  int iVar5;
  short in_AX;
  int iVar6;
  uint uVar7;
  int iVar8;
  float10 fVar9;
  undefined4 uVar10;
  float fVar11;

  iVar8 = in_AX * 0x1ea0 + DAT_006b2d98;
  if (*(char *)(in_AX * 0x1ea0 + 0x50 + DAT_006b2d98) == '\0') {
    fVar9 = (float10)fpatan((float10)DAT_007c3124,(float10)DAT_007c3120);
    *(float *)(iVar8 + 0x60) = (float)fVar9;
    fVar9 = (float10)fpatan((float10)DAT_007c3128,
                            SQRT((float10)DAT_007c3120 * (float10)DAT_007c3120 +
                                 (float10)DAT_007c3124 * (float10)DAT_007c3124));
    *(float *)(iVar8 + 100) = (float)fVar9;
    *(undefined4 *)(iVar8 + 0x70) = DAT_007c3114;
    *(undefined4 *)(iVar8 + 0x74) = DAT_007c3118;
    *(undefined4 *)(iVar8 + 0x78) = DAT_007c311c;
  }
  *(undefined4 *)(iVar8 + 0x68) = *(undefined4 *)(iVar8 + 0x60);
  *(undefined4 *)(iVar8 + 0x6c) = *(undefined4 *)(iVar8 + 100);
  *(undefined4 *)(iVar8 + 0x7c) = *(undefined4 *)(iVar8 + 0x70);
  *(undefined4 *)(iVar8 + 0x80) = *(undefined4 *)(iVar8 + 0x74);
  *(undefined4 *)(iVar8 + 0x84) = *(undefined4 *)(iVar8 + 0x78);
  fVar9 = (float10)fpatan((float10)DAT_007c3124,(float10)DAT_007c3120);
  *(float *)(iVar8 + 0x60) = (float)fVar9;
  fVar9 = (float10)fpatan((float10)DAT_007c3128,
                          SQRT((float10)DAT_007c3120 * (float10)DAT_007c3120 +
                               (float10)DAT_007c3124 * (float10)DAT_007c3124));
  *(float *)(iVar8 + 100) = (float)fVar9;
  *(undefined4 *)(iVar8 + 0x70) = DAT_007c3114;
  *(undefined4 *)(iVar8 + 0x74) = DAT_007c3118;
  *(undefined4 *)(iVar8 + 0x78) = DAT_007c311c;
  *(float *)(iVar8 + 0x54) = DAT_007c3120;
  *(float *)(iVar8 + 0x58) = DAT_007c3124;
  *(float *)(iVar8 + 0x5c) = DAT_007c3128;
  *(undefined1 *)(iVar8 + 0x50) = 1;
  if ((*(int *)(iVar8 + 8) != -1) && (iVar6 = object_try_and_get(4), iVar6 == 0)) {
    *(undefined4 *)(iVar8 + 8) = 0xffffffff;
  }
  if (*(uint *)(iVar8 + 8) == 0xffffffff) {
    return;
  }
  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar8 + 8) & 0xffff) * 0xc);
  iVar6 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar4 = *(int *)((*(uint *)(iVar6 + 0x478) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((*(int *)(iVar4 + 0x48) == 0) || (iVar5 = *(int *)(iVar4 + 0x4c), iVar5 == 0))
  goto LAB_00493c21;
  iVar1 = iVar8 + 0x8c;
  if (*(short *)(iVar8 + 0x16) == -1) {
    model_nodes_get_default_transforms(iVar1);
  }
  else {
    FUN_004d4a80(*(undefined2 *)(iVar8 + 0x18));
  }
  if ((0x11 < *(int *)(iVar5 + 0x10)) &&
     (sVar2 = *(short *)(*(int *)(iVar5 + 0x14) + 0x22), sVar2 != -1)) {
    if ((*(short *)(iVar6 + 0x4e2) == 2) &&
       ((*(short *)(DAT_006b2d98 + 0xc) == 0xd || (*(short *)(DAT_006b2d98 + 0xc) == 0xe)))) {
      uVar7 = (uint)(ushort)puVar3[0xae];
      if (0x2b < (short)((short)puVar3[0xad] - *(short *)((int)puVar3 + 0x2b2))) {
        iVar6 = __ftol();
        uVar7 = uVar7 + iVar6;
      }
    }
    else {
      uVar7 = (uint)(ushort)puVar3[0xae];
      if (*(short *)(sVar2 * 0xb4 + *(int *)(iVar4 + 0x78) + 0x22) <= (short)(ushort)puVar3[0xae])
      goto LAB_00493a36;
    }
    FUN_004d4f90(uVar7,iVar1);
  }
LAB_00493a36:
  if (*(short *)(iVar8 + 0x1a) != -1) {
    FUN_004d4f90(*(undefined2 *)(iVar8 + 0x1c),iVar1);
  }
  if (*(short *)(iVar8 + 0x20) != -1) {
    FUN_004d57d0(*(undefined4 *)(iVar8 + 0x24),(float)puVar3[0x91] + 0.5,iVar1);
  }
  if (((4 < *(int *)(iVar5 + 0x10)) && (sVar2 = *(short *)(*(int *)(iVar5 + 0x14) + 8), sVar2 != -1)
      ) && (8 < *(short *)(sVar2 * 0xb4 + *(int *)(iVar4 + 0x78) + 0x22))) {
    if (*(float *)(iVar8 + 0x30) <= 0.0) {
      if (*(float *)(iVar8 + 0x30) < 0.0) {
        fVar11 = -*(float *)(iVar8 + 0x30);
        uVar10 = 1;
        goto LAB_00493b0c;
      }
    }
    else {
      fVar11 = *(float *)(iVar8 + 0x30);
      uVar10 = 0;
LAB_00493b0c:
      FUN_004d51a0(uVar10,fVar11,iVar1);
    }
    if (*(float *)(iVar8 + 0x34) <= 0.0) {
      if (*(float *)(iVar8 + 0x34) < 0.0) {
        fVar11 = -*(float *)(iVar8 + 0x34);
        uVar10 = 2;
        goto LAB_00493b49;
      }
    }
    else {
      fVar11 = *(float *)(iVar8 + 0x34);
      uVar10 = 3;
LAB_00493b49:
      FUN_004d51a0(uVar10,fVar11,iVar1);
    }
    if (*(float *)(iVar8 + 0x40) <= 0.0) {
      if (*(float *)(iVar8 + 0x40) < 0.0) {
        fVar11 = -*(float *)(iVar8 + 0x40);
        uVar10 = 5;
        goto LAB_00493b86;
      }
    }
    else {
      fVar11 = *(float *)(iVar8 + 0x40);
      uVar10 = 4;
LAB_00493b86:
      FUN_004d51a0(uVar10,fVar11,iVar1);
    }
    if (*(float *)(iVar8 + 0x44) <= 0.0) {
      if (*(float *)(iVar8 + 0x44) < 0.0) {
        fVar11 = -*(float *)(iVar8 + 0x44);
        uVar10 = 6;
        goto LAB_00493bc3;
      }
    }
    else {
      fVar11 = *(float *)(iVar8 + 0x44);
      uVar10 = 7;
LAB_00493bc3:
      FUN_004d51a0(uVar10,fVar11,iVar1);
    }
    if (0.0 < *(float *)(iVar8 + 0x28)) {
      FUN_004d51a0(8,*(undefined4 *)(iVar8 + 0x28),iVar1);
    }
  }
  if (0 < *(short *)(iVar8 + 0x8a)) {
    model_nodes_blend_transforms
              (iVar8 + 0x88c,*(undefined2 *)(iVar8 + 0x88),*(short *)(iVar8 + 0x8a));
  }
LAB_00493c21:
  model_nodes_calculate_world_transforms(iVar8 + 0x108c,iVar8 + 0x8c,&DAT_007c3120,&DAT_007c312c);
  return;
}
#endif
