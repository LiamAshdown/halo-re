// unit_update_look_delta_controls  (Ghidra: FUN_0056e820; still unnamed, chosen from
//   functions.md's summary and the fields it writes)
// address 0x56e820, size 876 bytes
// name confidence: 0.3 (Sonnet phase2 pass proposed unit_update_look_delta_controls at 0.3;
//   kept as the best available name)
// rewrite confidence: 0.35
// evidence: types/objects.h object.parent_object (0x11c), .position/.forward/.up
//   (0x5c/0x74/0x80); types/units.h unit_data.vehicle_seat_index (0x2f0),
//   .animation_controls[3] (0x370), .unknown_34c/.unknown_358 (the cached point and its
//   per-frame delta, confirmed by the header's own comment citing this exact function);
//   types/tags.h Unit.seat_acceleration_scale (0x200) and Unit.seats (TagReflexive, pointer
//   field at 0x2e8), UnitSeat.marker_name (0x24) and UnitSeat.acceleration_scale (0x64,
//   decimal 100 -- matches the Ghidra literal exactly); types/objects.h object_marker
//   (transform 0x04, a real_matrix4x3) and types/math.h real_matrix4x3 (forward 0x04, up
//   0x1c) -- the stack layout puts the seat marker's node_transform.forward and
//   .up immediately after the 60 bytes Ghidra names local_6c, which is exactly the tail of
//   object_marker.transform plus the leading scale float of object_marker.node_transform.
// register convention: unit object index in EAX (in_EAX).
//   // blam-cc: EAX -> object_index
// UNSURE: the "+ 0.5" / "(uint)" pattern around each of the three stores into
//   animation_controls is Ghidra's rendering of a plain float store through a pointer it has
//   typed as uint*; it is reproduced here as ordinary float arithmetic (bias then scale) with
//   no integer truncation, because the values are immediately read back and clamped to [0,1] as
//   floats, which a genuine integer truncation would defeat.
// UNSURE: the "else" branch's call to object_get_position() has no visible arguments; read as
//   fetching the *parent* (vehicle) object's position into the same local slot the fallback
//   branch fills with the unit's own position, which is the only reading consistent with the
//   rest of the function treating the two branches as parallel (own position/basis vs. parent
//   position/seat-marker basis).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900

static float clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

// Updates unit_data.animation_controls[0..2] from the frame-to-frame change of a reference
// point, projected onto a forward/cross/up basis and scaled per-axis by a tag-defined
// acceleration_scale. When the unit is properly seated (valid parent and seat index), the
// reference point is the parent object's position and the basis comes from the seat's marker
// transform on the parent; otherwise the unit's own position and forward/up vectors are used.
void unit_update_look_delta_controls(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    real_point3d sample;
    real_vector3d forward, up;
    real_vector3d *scale;
    real_vector3d delta;

    if (unit->vehicle_seat_index == -1 || obj->parent_object == k_datum_index_none) {
        sample = obj->position;
        forward = obj->forward;
        up = obj->up;
        {
            Unit *tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
            scale = (real_vector3d *)((uint8_t *)tag + 0x200); // Unit.seat_acceleration_scale
        }
    } else {
        object *parent = ((object_header *)object_data->data)[obj->parent_object & 0xffff].data;
        Unit *parent_tag = (Unit *)tag_instances[parent->definition_tag & 0xffff].data;
        UnitSeat *seat = (UnitSeat *)((uint8_t *)*(uint8_t **)&((struct Unit *)parent_tag)->seats.pointer +
                                       unit->vehicle_seat_index * sizeof(UnitSeat));
        object_marker marker;
        int16_t found = object_get_node_local_transform(obj->parent_object, seat->marker_name.string,
                                                          &marker, 1);
        if (found == 0) {
            unit->animation_controls[0] = 0.5f;
            unit->animation_controls[1] = 0.5f;
            unit->animation_controls[2] = 0.5f;
            return;
        }
        object_get_position(&sample, obj->parent_object);
        forward = marker.node_transform.forward;
        up = marker.node_transform.up;
        scale = (real_vector3d *)&seat->acceleration_scale;
    }

    delta.i = sample.x - unit->unknown_34c.x;
    delta.j = sample.y - unit->unknown_34c.y;
    delta.k = sample.z - unit->unknown_34c.z;
    delta.i -= unit->unknown_358.i;
    delta.j -= unit->unknown_358.j;
    delta.k -= unit->unknown_358.k;

    unit->animation_controls[0] = clamp01((forward.i * delta.i + forward.j * delta.j + forward.k * delta.k) * scale->i + 0.5f);
    unit->animation_controls[1] = clamp01(((forward.k * up.j - forward.j * up.k) * delta.i +
                                            (up.k * forward.i - forward.k * up.i) * delta.j +
                                            (forward.j * up.i - forward.i * up.j) * delta.k) * scale->j + 0.5f);
    unit->animation_controls[2] = clamp01((up.j * delta.j + up.i * delta.i + up.k * delta.k) * scale->k + 0.5f);

    // The stored delta becomes (sample - old cached point), i.e. delta plus the *old*
    // unknown_358 that was just subtracted out of it above -- recovering that raw frame-to-frame
    // movement without a second read of the (now stale) cached point.
    {
        real_vector3d raw_movement;
        raw_movement.i = delta.i + unit->unknown_358.i;
        raw_movement.j = delta.j + unit->unknown_358.j;
        raw_movement.k = delta.k + unit->unknown_358.k;
        unit->unknown_34c = sample;
        unit->unknown_358 = raw_movement;
    }
}

#if 0
Original Ghidra decompilation (0x56e820):

void FUN_0056e820(void)

{
  float fVar1;
  float fVar2;
  uint *puVar3;
  uint uVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  short sVar9;
  uint in_EAX;
  float *pfVar10;
  int iVar11;
  undefined1 local_6c [60];
  float local_30;
  float local_2c;
  float local_28;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar4 = puVar3[0x47];
  if ((uVar4 == 0xffffffff) || ((short)puVar3[0xbc] == -1)) {
    local_c = (float)puVar3[0x17];
    local_8 = (float)puVar3[0x18];
    local_4 = (float)puVar3[0x19];
    local_30 = (float)puVar3[0x1d];
    local_2c = (float)puVar3[0x1e];
    local_28 = (float)puVar3[0x1f];
    local_18 = (float)puVar3[0x20];
    local_14 = (float)puVar3[0x21];
    local_10 = (float)puVar3[0x22];
    pfVar10 = (float *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x200);
  }
  else {
    iVar11 = (short)puVar3[0xbc] * 0x11c +
             *(int *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                           (uVar4 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                              DAT_0087bc14) + 0x2e8);
    sVar9 = object_get_node_local_transform(uVar4,iVar11 + 0x24,local_6c,1);
    if (sVar9 == 0) {
      puVar3[0xde] = 0x3f000000;
      puVar3[0xdd] = 0x3f000000;
      puVar3[0xdc] = 0x3f000000;
      return;
    }
    object_get_position();
    pfVar10 = (float *)(iVar11 + 100);
  }
  fVar5 = local_c - (float)puVar3[0xd3];
  fVar1 = (float)puVar3[0xd4];
  fVar2 = (float)puVar3[0xd5];
  fVar6 = fVar5 - (float)puVar3[0xd6];
  fVar7 = (local_8 - fVar1) - (float)puVar3[0xd7];
  fVar8 = (local_4 - fVar2) - (float)puVar3[0xd8];
  puVar3[0xdc] = (uint)((local_30 * fVar6 + local_2c * fVar7 + local_28 * fVar8) * *pfVar10 + 0.5);
  puVar3[0xdd] = (uint)(((local_28 * local_14 - local_2c * local_10) * fVar6 +
                        (local_10 * local_30 - local_28 * local_18) * fVar7 +
                        (local_2c * local_18 - local_30 * local_14) * fVar8) * pfVar10[1] + 0.5);
  puVar3[0xde] = (uint)((local_14 * fVar7 + local_18 * fVar6 + local_10 * fVar8) * pfVar10[2] + 0.5)
  ;
  if (0.0 <= (float)puVar3[0xdc]) {
    if ((float)puVar3[0xdc] <= 1.0) {
      uVar4 = puVar3[0xdc];
    }
    else {
      uVar4 = 0x3f800000;
    }
  }
  else {
    uVar4 = 0;
  }
  puVar3[0xdc] = uVar4;
  if (0.0 <= (float)puVar3[0xdd]) {
    if ((float)puVar3[0xdd] <= 1.0) {
      uVar4 = puVar3[0xdd];
    }
    else {
      uVar4 = 0x3f800000;
    }
  }
  else {
    uVar4 = 0;
  }
  puVar3[0xdd] = uVar4;
  if (0.0 <= (float)puVar3[0xde]) {
    if ((float)puVar3[0xde] <= 1.0) {
      uVar4 = puVar3[0xde];
    }
    else {
      uVar4 = 0x3f800000;
    }
  }
  else {
    uVar4 = 0;
  }
  puVar3[0xde] = uVar4;
  puVar3[0xd3] = (uint)local_c;
  puVar3[0xd4] = (uint)local_8;
  puVar3[0xd5] = (uint)local_4;
  puVar3[0xd6] = (uint)fVar5;
  puVar3[0xd7] = (uint)(local_8 - fVar1);
  puVar3[0xd8] = (uint)(local_4 - fVar2);
  return;
}
#endif
