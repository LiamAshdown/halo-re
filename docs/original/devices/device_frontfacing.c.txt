// device_frontfacing  (Ghidra: device_frontfacing, already named via a cea-pdb hint on the
// shared "front" string; functions.md: "Checks whether a nearby unit is facing the front side
// of a one-sided device, returning 0 (allowed) when the unit's forward vector points toward the
// device's front")
// address 0x44c130, size 112 bytes
// name confidence: 0.7   rewrite confidence: 0.55
// evidence: types/devices.h device_control_flags (_device_control_usable_from_both_sides_bit);
// types/objects.h object_marker (node_transform at 0x38, a real_matrix4x3 whose forward vector
// is at 0x04 within it, i.e. object_marker+0x3c), _object_mask_device_control (0x100);
// types/math.h real_vector3d.
// register convention: device object index in ESI (unaff_ESI), a caller-owned forward-vector
// pointer in EDI (unaff_EDI). Confirmed against disassembly (objdump 0x44c130-0x44c19f): ESI is
// pushed straight into ECX for object_try_and_get (established ECX -> object_index convention),
// and EDI is dereferenced only for the dot product, never written -- an input, not scratch.
//   // blam-cc: ESI -> device_index, EDI -> forward
// Ghidra's `undefined2` return is wrong: the two exits are `xor al,al` (0x44c192) and
// `mov al,bl` with BL seeded to 1 at 0x44c145, so this is an 8-bit bool in AL. uint8_t below is
// the correct width. The marker forward vector is read at [esp+0x40]/[esp+0x44]/[esp+0x48] with
// the marker buffer at [esp+0x04], i.e. marker+0x3c/+0x40/+0x44, which is exactly
// object_marker.node_transform (0x38) . real_matrix4x3.forward (0x04) -- confirmed, not inferred.
// Also resolved from that same disassembly: object_get_node_local_transform is called with
// flags=1, the literal string "front" (0x65edf4) as the marker name, a caller-local
// object_marker buffer, and device_index -- matching its established
// (object_index, marker_name, marker, flags) signature exactly; Ghidra elided every argument.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "devices.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t object_get_node_local_transform(uint32_t object_index, const char *marker_name,
    object_marker *marker, uint32_t flags); // 0x4f6080, established
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, established
    // blam-cc: ECX -> object_index, stack -> type_mask

uint8_t device_frontfacing(uint32_t device_index, real_vector3d *forward)
{
    object *control = object_try_and_get(device_index, _object_mask_device_control);

    if (control != (object *)0) {
        device_control_data *dev = (device_control_data *)((uint8_t *)control + sizeof(object));

        if ((dev->device.type_flags & (1u << _device_control_usable_from_both_sides_bit)) == 0) {
            object_marker marker;
            if (object_get_node_local_transform(device_index, "front", &marker, 1) == 1) {
                real_vector3d *marker_forward = &marker.node_transform.forward;
                if (0.0f < marker_forward->i * forward->i + marker_forward->j * forward->j +
                    marker_forward->k * forward->k) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x44c130), from tools/pack.py 0x44c130:

undefined2 device_frontfacing(void)

{
  short sVar1;
  int iVar2;
  float *unaff_EDI;
  float local_30;
  float local_2c;
  float local_28;

  iVar2 = object_try_and_get(0x100);
  if ((iVar2 != 0) && ((*(byte *)(iVar2 + 0x214) & 1) == 0)) {
    sVar1 = object_get_node_local_transform();
    if (sVar1 == 1) {
      if (0.0 < local_30 * *unaff_EDI + local_2c * unaff_EDI[1] + local_28 * unaff_EDI[2]) {
        return 0;
      }
    }
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
