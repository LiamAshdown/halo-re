// actor_grenade_trajectory_blocked  (Ghidra: actor_grenade_trajectory_blocked; named for this rewrite)
// address 0x42b190, size 218 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x42b190..0x42b269 (EAX direction, ECX actor, stack exclude / landing / out).)
// evidence: phase-4 summary ("checks whether a grenade's trajectory or landing radius
// intersects any nearby gathered actor other than the thrower") plus its two callees,
// ray_intersects_sphere_test and segment3d_within_radius_of_segment. Ghidra's own decompile
// is unusable here: it dropped every register argument to actor_gather_nearby_grenade_targets
// and both math callees and mis-split the local buffer into two unrelated-looking arrays.
// Rewritten from the real disassembly (objdump -d -M intel --start-address=0x42b190
// --stop-address=0x42b26a bin/halo.exe) against the entry layout confirmed in
// actor_grenade_avoidance_entry_init.c / actor_gather_nearby_grenade_targets.c.
// register convention: EAX -> trajectory_direction, ECX -> source_actor_index; stack ->
// exclude_object_index, landing_position, out_blocking_prop.
// blam-cc: EAX -> trajectory_direction, ECX -> source_actor_index, stack ->
// exclude_object_index, landing_position, out_blocking_prop
//
// UNSURE: entry+0x04..0x0f (target_position below) is never written by
// actor_gather_nearby_grenade_targets or actor_grenade_avoidance_entry_init in this batch,
// yet this function reads it as a real_point3d for both math calls, and FUN_0055a2e0 (called
// from actor_grenade_avoidance_entry_init) itself does a read-modify-write on part of it
// through a pointer it only has because EAX happens to still hold &entry+4 two calls up the
// stack -- a "sibling register passthrough" the same way this project's existing
// segment3d_within_radius_of_segment.c documents for its own irreducible case. Left as a
// named but unverified field; whatever fills it lives outside this batch's address range.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t actor_gather_nearby_grenade_targets(datum_index source_actor_index, int16_t maximum_count,
                                                     ai_grenade_avoidance_entry *out_entries); // 0x0042afc0
extern uint8_t ray_intersects_sphere_test(real_point3d *center, real_point3d *origin, real_vector3d *direction, real radius); // 0x4ce6c0
extern int segment3d_within_radius_of_segment(real_point3d *a_start, real_point3d *b_start, real_vector3d *a_direction, real_vector3d *b_direction, real radius); // 0x4ceae0

// blam-cc: EAX -> trajectory_direction, ECX -> source_actor_index, stack ->
// exclude_object_index, landing_position, out_blocking_prop
// Gathers up to 32 nearby friendly targets for source_actor_index, then checks each one
// (other than exclude_object_index) against the grenade's flight: if the target's crouch
// offset already reads as zero, tests a sphere at the target's (unestablished) position
// against the trajectory ray; otherwise tests a short vertical segment above the target
// against the trajectory segment. Returns 1 if nothing is in the way, 0 if something is,
// and writes the blocking target's prop index through out_blocking_prop (when non-NULL;
// left at -1 when nothing blocks).
uint8_t actor_grenade_trajectory_blocked(real_vector3d *trajectory_direction, datum_index source_actor_index,
                                          datum_index exclude_object_index, real_point3d *landing_position,
                                          int32_t *out_blocking_prop)
{
    ai_grenade_avoidance_entry entries[32];
    int16_t count;
    int16_t i;
    uint8_t clear;
    int32_t blocking_prop;
    uint8_t hit;

    clear = 1;
    blocking_prop = -1;
    count = actor_gather_nearby_grenade_targets(source_actor_index, 32, entries);

    for (i = 0; i < count; i++) {
        if (entries[i].object_index == exclude_object_index) {
            continue;
        }
        if (entries[i].already_clear) {
            hit = ray_intersects_sphere_test(&entries[i].target_position, landing_position,
                                              trajectory_direction, entries[i].avoid_until);
        } else {
            real_vector3d target_offset;
            target_offset.i = 0.0f;
            target_offset.j = 0.0f;
            target_offset.k = entries[i].crouch_offset;
            hit = (uint8_t)segment3d_within_radius_of_segment(landing_position, &entries[i].target_position,
                                                                trajectory_direction, &target_offset,
                                                                entries[i].avoid_until);
        }
        if (hit) {
            blocking_prop = entries[i].prop_index;
            clear = 0;
            break;
        }
    }

    if (out_blocking_prop != 0) {
        *out_blocking_prop = blocking_prop;
    }
    return clear;
}

#if 0
Original Ghidra decompilation (0x42b190):

undefined1 FUN_0042b190(int param_1,undefined4 param_2,int *param_3)

{
  char cVar1;
  short sVar2;
  int iVar3;
  short sVar4;
  undefined1 local_509;
  int local_508;
  char local_500 [28];
  int aiStack_4e4 [313];

  local_509 = 1;
  local_508 = -1;
  sVar2 = actor_gather_nearby_grenade_targets();
  sVar4 = 0;
  if (0 < sVar2) {
    do {
      iVar3 = (int)sVar4;
      if (aiStack_4e4[iVar3 * 10 + 1] != param_1) {
        if (local_500[iVar3 * 0x28] == '\0') {
          cVar1 = segment3d_within_radius_of_segment(param_2,aiStack_4e4[iVar3 * 10 + 2]);
        }
        else {
          cVar1 = ray_intersects_sphere_test(aiStack_4e4[iVar3 * 10 + 2]);
        }
        if (cVar1 != '\0') {
          local_508 = aiStack_4e4[sVar4 * 10];
          local_509 = 0;
          break;
        }
      }
      sVar4 = sVar4 + 1;
    } while (sVar4 < sVar2);
  }
  if (param_3 != (int *)0x0) {
    *param_3 = local_508;
  }
  return local_509;
}

Real disassembly (0x42b190-0x42b269), used in place of the above because it dropped every
register argument to actor_gather_nearby_grenade_targets, ray_intersects_sphere_test and
segment3d_within_radius_of_segment:

0042b190: sub    esp,0x50c
0042b196: push   ebp
0042b197: push   esi
0042b198: push   edi
0042b199: mov    esi,eax                  ; esi = trajectory_direction (incoming EAX)
0042b19b: lea    eax,[esp+0x18]           ; eax = &entries[0]
0042b19f: push   eax
0042b1a0: push   0x20
0042b1a2: push   ecx                      ; ecx = source_actor_index (incoming ECX)
0042b1a3: mov    byte ptr [esp+0x1b],0x1  ; clear = 1
0042b1a8: mov    dword ptr [esp+0x1c],0xffffffff  ; blocking_prop = -1
0042b1b0: call   0x42afc0
0042b1b5: add    esp,0xc
0042b1b8: xor    ebp,ebp                  ; i = 0
0042b1ba: mov    edi,eax                  ; count
0042b1bc: test   di,di
0042b1bf: mov    [esp+0x14],edi
0042b1c3: jle    0x42b24b
0042b1c9: push   ebx
0042b1d0: mov    edx,[esp+0x520]          ; edx = exclude_object_index
0042b1d7: movsx  eax,bp
0042b1da: lea    eax,[eax+eax*4]
0042b1dd: shl    eax,0x3                  ; eax = i * sizeof(entry)
0042b1e0: cmp    [esp+eax*1+0x3c],edx     ; entries[i].object_index == exclude_object_index
0042b1e4: je     0x42b22f
0042b1e6: mov    cl,[esp+eax*1+0x1c]      ; entries[i].already_clear
0042b1ea: test   cl,cl
0042b1ec: je     0x42b20a
0042b1ee: mov    ecx,[esp+eax*1+0x40]     ; entries[i].avoid_until
0042b1f2: push   ecx
0042b1f3: mov    ecx,[esp+0x528]          ; ecx = landing_position
0042b1fa: lea    eax,[esp+eax*1+0x24]     ; eax = &entries[i].target_position
0042b1fe: mov    edx,esi                  ; edx = trajectory_direction
0042b200: call   0x4ce6c0                 ; ray_intersects_sphere_test(eax, ecx, edx, [esp])
0042b205: add    esp,0x4
0042b208: jmp    0x42b22b
0042b20a: mov    edx,[esp+eax*1+0x40]     ; entries[i].avoid_until
0042b20e: lea    edi,[esp+eax*1+0x2c]     ; edi = &entries[i].unknown_10 (as {0,0,crouch_offset})
0042b212: lea    ebx,[esp+eax*1+0x20]     ; ebx = &entries[i].target_position
0042b216: mov    eax,[esp+0x524]          ; eax = landing_position
0042b21d: push   edx
0042b21e: push   eax
0042b21f: call   0x4ceae0                 ; segment3d_within_radius_of_segment(eax,ebx,esi,edi,[esp])
0042b224: mov    edi,[esp+0x20]
0042b228: add    esp,0x8
0042b22b: test   al,al
0042b22d: jne    0x42b237
0042b22f: inc    ebp
0042b230: cmp    bp,di
0042b233: jl     0x42b1d0
0042b235: jmp    0x42b24a
0042b237: movsx  eax,bp
0042b23a: lea    ecx,[eax+eax*4]
0042b23d: mov    edx,[esp+ecx*8+0x38]     ; entries[i].prop_index
0042b241: mov    byte ptr [esp+0x13],0x0  ; clear = 0
0042b246: mov    [esp+0x14],edx           ; blocking_prop = entries[i].prop_index
0042b24a: pop    ebx
0042b24b: mov    eax,[esp+0x524]          ; out_blocking_prop
0042b252: test   eax,eax
0042b254: pop    edi
0042b255: pop    esi
0042b256: pop    ebp
0042b257: je     0x42b25f
0042b259: mov    ecx,[esp+0x4]
0042b25d: mov    [eax],ecx
0042b25f: mov    al,[esp+0x3]             ; return clear
0042b263: add    esp,0x50c
0042b269: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
