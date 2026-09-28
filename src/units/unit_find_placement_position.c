// unit_find_placement_position  (Ghidra: unit_find_placement_position, renamed)
// address 0x55a500, size 1299 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// REWRITTEN from objdump 0x55a500..0x55aa12. Stack: (unit, reference object, out_position, radius, grid_mode,
//   skip_reposition, scale_radius); EDX: an optional start point (else the unit's position). EAX is not read.
//   With no unit the reference object stands in for the pill / tag lookups (and nothing is moved). Candidates:
//   the 27-entry offset table 0x65e660 in the unit's forward / side / up frame (grid_mode) or its first 18 in
//   world axes, scaled by radius (times the pill radius with scale_radius). A candidate must be inside the
//   structure (leaf with a cluster), get a clear pill position (0x507170), not hit the structure along the pill
//   (0x506040), nor the reference object's collision (0x5050b0) and see the reference object's centre both ways
//   (0x401a20, a hit only on the other object). The winner, lowered by the biped's collision radius (+0x42c)
//   unless tag +0x2f4 bit 3, becomes the unit's position (relinked into its leaf unless skip_reposition) and
//   *out_position. The draft called six of these helpers with missing operands.
// blam-cc: stack -> anchor_object, orientation_object, out_position, radius, grid_mode, skip_reposition,
//   scale_radius; EDX -> reference_direction (a start point)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "projectiles.h" // collision_result
#include "physics.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern real_vector3d *global_up3d_pointer; // 0x00696720
extern ModelCollisionGeometryBSP *global_collision_bsp; // 0x00746f90
extern uint8_t *global_structure_bsp; // 0x00746f9c (+0xe4 leaves, 0x10 each, +0x8 cluster word)
extern real_vector3d placement_offset_table[27]; // 0x0065e660

extern void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height,
    float *pill_radius_out); // 0x55a2e0, EAX, ECX, stack, EBX
extern uint8_t object_collision_context_build(uint32_t object_index, object_collision_context *out_context); // 0x504e10, EDI, ECX
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, EAX, ECX, EDX
extern uint8_t physics_point_find_clear_position(uint32_t flags, real_point3d *current_position, float sample_radius,
    float x_margin, float y_margin, uint32_t exclude_object_index, real_point3d *out_position); // 0x507170, ESI, stack
extern uint8_t collision_test_movement_pill(uint32_t flags, real_point3d *origin, float radius, real_vector3d *delta,
    collision_result *result); // 0x506040, stack, EDI, ESI
extern uint8_t object_collision_context_test_pill(object_collision_context *context, real_point3d *origin,
    real_vector3d *delta, float radius_scale, object_node_collision_result *out_result); // 0x5050b0
extern uint8_t collision_test_movement_segment_between_points(real_point3d *origin, real_point3d *target,
    uint32_t flags, uint32_t exclude_object_index, collision_result *result); // 0x401a20, EAX, ECX, stack
extern void scenario_location_from_point(bsp_leaf_reference *out, real_point3d *point); // 0x53e780, ESI, EDX
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index,
    bsp_leaf_reference *location); // 0x4f5350, ESI, EDI, stack

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

uint32_t unit_find_placement_position(uint32_t anchor_object, uint32_t orientation_object, real_point3d *out_position,
                                      float radius, char grid_mode, char skip_reposition, char scale_radius,
                                      uint32_t object_index_a, real_vector3d *reference_direction)
{
    uint8_t found = 0;                          // [esp+0x12]
    uint8_t borrowed_anchor = 0;                // [esp+0x13]
    real_point3d center;                        // [esp+0x5c] the reference object's bounding centre
    real_point3d base;                          // [esp+0x30]
    real_point3d scratch;                       // [esp+0x50] (the pill query's position when a start point is given)
    float pill_height;                          // [esp+0x24]
    float pill_radius;                          // [esp+0x28]
    uint32_t flags;                             // [esp+0x2c]
    int16_t count;                              // [esp+0x4c]
    real_vector3d side;                         // [esp+0x40]
    real_vector3d vertical;                     // [esp+0x50]
    object_collision_context context;           // [esp+0x70]
    collision_result segment_result;            // [esp+0x80]
    collision_result pill_result;               // [esp+0xd0]
    object_node_collision_result context_result; // [esp+0x120]
    bsp_leaf_reference location;                // [esp+0x68]
    uint8_t *unit;                              // ebp
    uint8_t *tag;
    int16_t i;

    (void)object_index_a;
    if (anchor_object == k_datum_index_none) {
        if (orientation_object == k_datum_index_none) {
            return 0;
        }
    }
    if (orientation_object != k_datum_index_none) {
        center = *(real_point3d *)(OBJECT_DATA(orientation_object) + 0xa0);
    }
    if (anchor_object == k_datum_index_none) {
        anchor_object = orientation_object;
        borrowed_anchor = 1;
    }
    unit = OBJECT_DATA(anchor_object);
    tag = (uint8_t *)tag_instances[*(datum_index *)unit & 0xffff].data;
    flags = (*(uint32_t *)(tag + 0x2f4) & 0x20) ? 0xc2a0 : 0x20c3a0;
    if (reference_direction != 0) {
        base = *(real_point3d *)reference_direction;
        unit_get_crouch_height_offset(&scratch, anchor_object, &pill_height, &pill_radius);
    } else {
        unit_get_crouch_height_offset(&base, anchor_object, &pill_height, &pill_radius);
    }
    if (borrowed_anchor) {
        anchor_object = k_datum_index_none;
    }
    count = grid_mode ? 27 : 18;
    if (orientation_object != k_datum_index_none) {
        object_collision_context_build(orientation_object, &context);
    }
    {
        real_vector3d *f = (real_vector3d *)(unit + 0x74);
        real_vector3d *u = (real_vector3d *)(unit + 0x80);

        side.i = u->k * f->j - f->k * u->j;
        side.j = f->k * u->i - u->k * f->i;
        side.k = u->j * f->i - u->i * f->j;
        vector3d_normalize_with_length(&side);
    }
    vertical.i = pill_height * global_up3d_pointer->i;
    vertical.j = pill_height * global_up3d_pointer->j;
    vertical.k = pill_height * global_up3d_pointer->k;
    if (scale_radius) {
        radius = pill_radius * radius;
    }
    for (i = 0; i < count && !found; i++) {
        real_vector3d *offset = &placement_offset_table[i];
        real_point3d point;                     // [esp+0x14]
        int32_t leaf;

        if (grid_mode) {
            real_vector3d *f = (real_vector3d *)(unit + 0x74);
            real_vector3d *u = (real_vector3d *)(unit + 0x80);
            float a = radius * offset->i;
            float b = radius * offset->j;
            float c = radius * offset->k;

            point.x = a * f->i + base.x + side.i * b + c * u->i;
            point.y = a * f->j + base.y + side.j * b + c * u->j;
            point.z = a * f->k + base.z + side.k * b + c * u->k;
        } else {
            point.x = radius * offset->i + base.x;
            point.y = radius * offset->j + base.y;
            point.z = radius * offset->k + base.z;
        }
        leaf = (int32_t)bsp3d_node_find_leaf(0, global_collision_bsp, &point);
        if (leaf == -1) {
            continue;
        }
        if (*(int16_t *)(*(uint8_t **)(global_structure_bsp + 0xe4) + (leaf & 0x7fffffff) * 0x10 + 0x8) == -1) {
            continue;
        }
        if (!physics_point_find_clear_position(flags, &point, pill_radius + pill_radius, pill_height, pill_radius,
                anchor_object, &point)) {
            continue;
        }
        if (collision_test_movement_pill(flags, &point, pill_radius, &vertical, &pill_result)) {
            continue;
        }
        if (orientation_object != k_datum_index_none) {
            if (object_collision_context_test_pill(&context, &point, &vertical, pill_radius, &context_result)) {
                continue;
            }
            if (collision_test_movement_segment_between_points(&point, &center, flags, anchor_object, &segment_result) &&
                segment_result.object_index != orientation_object) {
                continue;
            }
            if (collision_test_movement_segment_between_points(&center, &point, flags, orientation_object, &segment_result) &&
                segment_result.object_index != anchor_object) {
                continue;
            }
        }
        scenario_location_from_point(&location, &point);
        if (!(*(uint32_t *)(tag + 0x2f4) & 0x8)) {
            point.z = point.z - *(float *)(tag + 0x42c);
        }
        if (anchor_object != k_datum_index_none && !skip_reposition) {
            *(real_point3d *)&((unit_object *)unit)->base.position.x = point;
            object_recalculate_bounding_radius_recursive(anchor_object);
            object_set_position_and_relink(&point, anchor_object, &location);
        }
        if (out_position != 0) {
            *out_position = point;
        }
        found = 1;
    }
    return found;
}

#if 0
Original Ghidra decompilation (0x55a500):

uint FUN_0055a500(uint param_1,uint param_2,float *param_3,float param_4,char param_5,char param_6,
                 char param_7)

{
  uint *puVar1;
  float fVar2;
  float fVar3;
  char cVar4;
  uint in_EAX;
  float *in_EDX;
  int iVar5;
  bool bVar6;
  char local_536;
  float local_534;
  float local_530;
  float local_52c;
  float local_528;
  float local_524;
  float local_520;
  int local_51c;
  float local_518;
  float local_514;
  float local_510;
  int local_50c;
  float local_508;
  float local_504;
  float local_500;
  int local_4fc;
  float local_4f8;
  float local_4f4;
  float local_4f0;
  undefined4 local_4ec;
  undefined4 local_4e8;
  undefined4 local_4e4;
  undefined1 local_4e0 [8];
  undefined1 local_4d8 [16];
  undefined1 local_4c8 [56];
  uint local_490;
  undefined1 local_428 [1064];

  local_536 = '\0';
  if (param_1 == 0xffffffff) {
    if (param_2 == 0xffffffff) {
      return in_EAX & 0xffffff00;
    }
  }
  else if (param_2 == 0xffffffff) goto LAB_0055a568;
  iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
  local_4ec = *(undefined4 *)(iVar5 + 0xa0);
  local_4e8 = *(undefined4 *)(iVar5 + 0xa4);
  local_4e4 = *(undefined4 *)(iVar5 + 0xa8);
LAB_0055a568:
  bVar6 = param_1 == 0xffffffff;
  if (bVar6) {
    param_1 = param_2;
  }
  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  local_51c = (-(uint)((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f4)
                       & 0x20) != 0) & 0xffdfff00) + 0x20c3a0;
  if (in_EDX != (float *)0x0) {
    local_518 = *in_EDX;
    local_514 = in_EDX[1];
    local_510 = in_EDX[2];
  }
  FUN_0055a2e0(&local_524);
  if (bVar6) {
    param_1 = 0xffffffff;
  }
  iVar5 = 0x1b - ((param_5 != '\0') - 1 & 9);
  local_4fc = iVar5;
  if (param_2 != 0xffffffff) {
    FUN_00504e10();
  }
  local_534 = (float)puVar1[0x22] * (float)puVar1[0x1e] - (float)puVar1[0x1f] * (float)puVar1[0x21];
  local_530 = (float)puVar1[0x1f] * (float)puVar1[0x20] - (float)puVar1[0x22] * (float)puVar1[0x1d];
  local_52c = (float)puVar1[0x21] * (float)puVar1[0x1d] - (float)puVar1[0x20] * (float)puVar1[0x1e];
  local_508 = local_534;
  local_504 = local_530;
  local_500 = local_52c;
  vector3d_normalize_with_length();
  local_4f8 = local_524 * *(float *)PTR_DAT_00696720;
  local_4f4 = local_524 * *(float *)(PTR_DAT_00696720 + 4);
  local_4f0 = local_524 * *(float *)(PTR_DAT_00696720 + 8);
  if (param_7 != '\0') {
    param_4 = local_520 * param_4;
  }
  local_50c = 0;
  do {
    if ((short)iVar5 <= (short)local_50c) break;
    iVar5 = (int)(short)local_50c;
    if (param_5 == '\0') {
      local_534 = param_4 * (float)(&DAT_0065e660)[iVar5 * 3] + local_518;
      local_530 = param_4 * (float)(&DAT_0065e664)[iVar5 * 3] + local_514;
      local_52c = param_4 * (float)(&DAT_0065e668)[iVar5 * 3] + local_510;
    }
    else {
      fVar2 = param_4 * (float)(&DAT_0065e660)[iVar5 * 3];
      fVar3 = param_4 * (float)(&DAT_0065e664)[iVar5 * 3];
      local_528 = param_4 * (float)(&DAT_0065e668)[iVar5 * 3];
      local_534 = local_528 * (float)puVar1[0x20] +
                  local_508 * fVar3 + fVar2 * (float)puVar1[0x1d] + local_518;
      local_530 = local_528 * (float)puVar1[0x21] +
                  local_504 * fVar3 + fVar2 * (float)puVar1[0x1e] + local_514;
      local_52c = local_528 * (float)puVar1[0x22] +
                  local_500 * fVar3 + fVar2 * (float)puVar1[0x1f] + local_510;
    }
    iVar5 = FUN_005013a0();
    fVar2 = local_520;
    if ((iVar5 != -1) && (*(short *)(iVar5 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4)) != -1)) {
      cVar4 = FUN_00507170(local_51c,local_520 + local_520,local_524,local_520,param_1,&local_534);
      if (cVar4 != '\0') {
        cVar4 = FUN_00506040(local_51c,&local_534,fVar2);
        if ((cVar4 == '\0') &&
           ((param_2 == 0xffffffff ||
            (((cVar4 = FUN_005050b0(local_4d8,&local_534,&local_4f8,fVar2,local_428), cVar4 == '\0'
              && ((cVar4 = FUN_00401a20(local_51c,param_1,local_4c8), cVar4 == '\0' ||
                  (local_490 == param_2)))) &&
             ((cVar4 = FUN_00401a20(local_51c,param_2,local_4c8), cVar4 == '\0' ||
              (local_490 == param_1)))))))) {
          iVar5 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
          FUN_0053e780();
          if ((*(byte *)(iVar5 + 0x2f4) & 8) == 0) {
            local_52c = local_52c - *(float *)(iVar5 + 0x42c);
          }
          if ((param_1 != 0xffffffff) && (param_6 == '\0')) {
            puVar1[0x17] = (uint)local_534;
            puVar1[0x18] = (uint)local_530;
            puVar1[0x19] = (uint)local_52c;
            object_recalculate_bounding_radius_recursive(param_1);
            object_set_position_and_relink(local_4e0);
          }
          if (param_3 != (float *)0x0) {
            *param_3 = local_534;
            param_3[1] = local_530;
            param_3[2] = local_52c;
          }
          local_536 = '\x01';
        }
      }
    }
    local_50c = local_50c + 1;
    iVar5 = local_4fc;
  } while (local_536 == '\0');
  return CONCAT31((int3)((uint)local_50c >> 8),local_536);
}
#endif
