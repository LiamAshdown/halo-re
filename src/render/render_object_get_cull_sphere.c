// render_object_get_cull_sphere  (not a Ghidra function: code at 0x50e8d0 that Ghidra never
// created because it is only referenced as a callback pointer, pushed at 0x50ead5 / 0x50eb14)
// address 0x50e8d0, size 86 bytes (0x50e8d0..0x50e925)
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: objdump -d -M intel 0x50e8d0..0x50e925. structure_bsp_collect_visible_objects
//   0x554420 calls it as bounds(handle, &center_buffer, &radius) (pushes at 0x554474..0x55447e:
//   radius local esp+0x10, then the 12 byte centre buffer esp+0x18, then the handle) and then
//   hands the centre buffer to render_frustum_test_sphere in EDX. The body copies
//   object.bounding_center (+0xa0, 3 dwords) and the Object tag render_bounding_radius (+0x104).
// register convention: cdecl, three stack arguments.
//   // blam-cc: cdecl (callback)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data;     // 0x008603b0, objects module
extern tag_instance *tag_instances; // 0x0087bc14, cache module

// Frustum culling sphere of an object for render_objects_collect: its bounding centre and the
// render bounding radius of its Object tag.
void render_object_get_cull_sphere(datum_index object_index, real_point3d *center, float *radius)
{
    object_header *header = &((object_header *)object_data->data)[(uint16_t)object_index];
    object *o = header->data;
    Object *definition;

    *center = o->bounding_center;
    definition = (Object *)tag_instances[(uint16_t)o->definition_tag].data;
    *radius = definition->render_bounding_radius;
}

#if 0
Original Ghidra decompilation (0x50e8d0): none. Ghidra has no function at this address; the
disassembly it was rewritten from:

  50e8d0:	mov    eax,DWORD PTR [esp+0x4]
  50e8d4:	mov    ecx,DWORD PTR ds:0x8603b0
  50e8da:	mov    edx,DWORD PTR [ecx+0x34]
  50e8dd:	and    eax,0xffff
  50e8e2:	lea    eax,[eax+eax*2]
  50e8e5:	mov    eax,DWORD PTR [edx+eax*4+0x8]
  50e8e9:	mov    edx,DWORD PTR [esp+0x8]
  50e8ed:	lea    ecx,[eax+0xa0]
  50e8f3:	push   esi
  50e8f4:	mov    esi,DWORD PTR [ecx]
  50e8f6:	mov    DWORD PTR [edx],esi
  50e8f8:	mov    esi,DWORD PTR [ecx+0x4]
  50e8fb:	mov    DWORD PTR [edx+0x4],esi
  50e8fe:	mov    ecx,DWORD PTR [ecx+0x8]
  50e901:	mov    DWORD PTR [edx+0x8],ecx
  50e904:	mov    edx,DWORD PTR [eax]
  50e906:	mov    eax,ds:0x87bc14
  50e90b:	and    edx,0xffff
  50e911:	shl    edx,0x5
  50e914:	mov    ecx,DWORD PTR [edx+eax*1+0x14]
  50e918:	mov    edx,DWORD PTR [ecx+0x104]
  50e91e:	mov    eax,DWORD PTR [esp+0x10]
  50e922:	mov    DWORD PTR [eax],edx
  50e924:	pop    esi
  50e925:	ret
#endif
