// hud_draw_damage_indicators  (Ghidra: FUN_004b14c0, still unnamed there; named here)
// address 0x4b14c0, size 520 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.65
// evidence: out/phase4/interface_functions.md "Draws up to four fixed-direction on-screen
// indicator icons (e.g. edge-of-screen pings) around the viewport border."; the fade/read
// call at 0x457220 is player_effect_fade_damage_indicators (src/effects/), whose out
// parameter is the pre-fade snapshot of player_effect::damage_indicator_alpha[4]
// (types/effects.h, +0xe4); the "no unit" early-out writes a single dword 0 over exactly
// that same 4-byte array (local_player_index * 0xec + 0xe4 + 0x006f1884). Rewritten
// directly from objdump -d 0x4b14c0..0x4b16c7 because Ghidra's decompile drops every
// register argument and mis-decodes the split-screen selector as a CONCAT22 sign-extend
// that the disassembly does not actually perform (it is a plain zero-extending word load).
// register convention: local player index in EAX, read only as its low 16 bits.
//   // blam-cc: local_player_index -> EAX
// UNSURE: the exact compass/edge meaning of direction index 0..3 (matched 1:1 against
// player_effect::damage_indicator_alpha[i], itself set elsewhere by
// player_effect_mark_damage_direction) is not recovered here.
// UNSURE: the hud_globals tag fields this reads -- +0x310 (four int16 edge offsets, one per
// direction), +0x344 (bitmap tag id), +0x348/+0x34a (single- vs split-screen sequence
// index) and +0x34c (packed color) -- have no header type yet (types/tags.h has no
// HudGlobals struct), so they are read as raw byte offsets off hud_globals_tag_data.
// Phase-4 review of s2 part 2: 0x4acbb0 is hud_draw_bitmap_at (rewritten); its fourth stack
// argument is the rotation in radians (0, PI/2, PI, 3*PI/2 per direction) and anchor 4 is
// the center of the icon (hud_bitmap_anchor_extents). The four direction cases were checked
// against the jump table at 0x4b16c8. HUDGlobals (types/tags.h) now exists; the raw offsets
// are hud_damage_* fields of it.
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "effects.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data;                    // 0x0087a480, stride 0x200 (no types/players.h yet)
extern player_globals *local_player_globals;       // 0x0087a478
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern player_effect_globals *player_effect_globals_pointer; // 0x006f1884
extern int16_t render_viewport_top;                // 0x007c3140
extern int16_t render_viewport_left;               // 0x007c3142
extern float hud_damage_indicator_screen_center_x; // 0x0067321c (== 320.0f, half of 640)

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern void player_effect_fade_damage_indicators(int16_t local_player_index,
                                                  uint32_t *out_previous_indicators); // 0x457220, blam-cc: EAX index, EDX out
extern void hud_meter_resolve_bitmap_frame(datum_index bitmap_tag, int16_t sequence_index, uint16_t frame_index,
                                           void **out_data, int32_t *out_offset); // 0x4ab8d0, blam-cc: EAX frame_index
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x444550, blam-cc: EAX bitmap
extern void hud_draw_bitmap_at(const float *uv, BitmapData *bitmap, uint8_t pixel_uvs, int16_t anchor,
                               const Point2DInt *screen_position, float scale, float rotation, uint32_t color); // 0x4acbb0, blam-cc: EAX uv, EDX bitmap, CL pixel_uvs

// blam-cc: local_player_index -> EAX
// Draws the local player's four directional damage indicators (edge-of-screen arrows) for
// every direction whose pre-fade alpha byte is still in its "recently hit" window (1..0x1d);
// the value 0 means off and >= 0x1e means fully faded/invisible. Called once per frame per
// local player; hides every indicator in one write when the player currently has no unit.
void hud_draw_damage_indicators(int16_t local_player_index)
{
    datum_index unit_index;
    object *unit;
    uint32_t previous_indicators; // the alpha[4] snapshot from just before this frame's fade
    uint8_t *previous_bytes = (uint8_t *)&previous_indicators;
    int direction;

    if (local_player_index == -1) {
        return;
    }

    if (local_player_index >= 1) {
        // no such local player slot on retail PC (player_globals::local_players has 1 entry)
        unit_index = (datum_index)-1;
    } else {
        datum_index player_index = local_player_globals->local_players[local_player_index];
        if (player_index == (datum_index)-1) {
            unit_index = (datum_index)-1;
        } else {
            player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
            unit_index = p->unit;
        }
    }

    unit = object_try_and_get(unit_index, 3);
    if (unit == 0) {
        // no controlled unit: hide all four indicators with a single dword write
        *(uint32_t *)player_effect_globals_pointer->players[local_player_index].damage_indicator_alpha = 0;
        return;
    }

    {
        uint8_t *hud = (uint8_t *)hud_globals_tag_data;
        uint8_t *edge_offsets = hud + 0x310;    // four int16 edge offsets, one per direction
        datum_index icon_bitmap = *(datum_index *)(hud + 0x344);
        uint16_t sequence_index = (local_player_globals->local_player_count <= 1)
            ? *(uint16_t *)(hud + 0x348)
            : *(uint16_t *)(hud + 0x34a);
        uint32_t icon_color = *(uint32_t *)(hud + 0x34c);

        player_effect_fade_damage_indicators(local_player_index, &previous_indicators);

        for (direction = 0; direction < 4; direction++) {
            float x, y;
            uint32_t rotation_bits;
            void *bitmap_data = 0;
            int32_t sprite_rect = 0;
            Point2DInt position;

            if (previous_bytes[direction] == 0 || previous_bytes[direction] >= 0x1e) {
                continue;
            }

            switch (direction) {
            case 1:
                x = (float)(*(int16_t *)(edge_offsets + 4) + 8);
                y = 240.0f;
                rotation_bits = 0x3fc90fdb; // pi/2
                break;
            case 2:
                x = hud_damage_indicator_screen_center_x;
                y = (float)(0x1d8 - *(int16_t *)(edge_offsets + 2));
                rotation_bits = 0;
                break;
            case 3:
                x = (float)(0x278 - *(int16_t *)(edge_offsets + 6));
                y = 240.0f;
                rotation_bits = 0x4096cbe4; // 3*pi/2
                break;
            default: // direction == 0
                x = hud_damage_indicator_screen_center_x;
                y = (float)(*(int16_t *)edge_offsets + 8);
                rotation_bits = 0x40490fdb; // pi
                break;
            }
            x -= (float)render_viewport_left;
            y -= (float)render_viewport_top;

            hud_meter_resolve_bitmap_frame(icon_bitmap, (int16_t)sequence_index, 0, &bitmap_data, &sprite_rect);
            if (bitmap_data == 0) {
                continue;
            }
            if (texture_cache_get((BitmapData *)bitmap_data, 0, 1) == 0) {
                continue;
            }
            position.x = (int16_t)(int32_t)x;
            position.y = (int16_t)(int32_t)y;
            hud_draw_bitmap_at((const float *)sprite_rect, (BitmapData *)bitmap_data, 0, 4, &position, 1.0f,
                               *(float *)&rotation_bits, icon_color);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4b14c0) -- register arguments dropped, split-screen
selector mis-decoded as a CONCAT22 sign-extend the disassembly does not perform; the
rewrite above follows objdump -d 0x4b14c0..0x4b16c7 instead:

void FUN_004b14c0(void)

{
  int iVar1;
  short in_AX;
  int iVar2;
  uint uVar3;
  ushort uVar4;
  short *psVar5;
  undefined4 local_1c;
  int local_18;
  undefined2 local_14;
  undefined2 local_12;
  undefined4 local_10;
  byte local_c [4];
  float local_8;
  float local_4;

  if (in_AX != -1) {
    iVar2 = object_try_and_get(3);
    iVar1 = DAT_0071941c;
    if (iVar2 == 0) {
      *(undefined4 *)(in_AX * 0xec + 0xe4 + DAT_006f1884) = 0;
      return;
    }
    psVar5 = (short *)(DAT_0071941c + 0x310);
    FUN_00457220();
    uVar4 = 0;
    do {
      if ((local_c[(short)uVar4] == 0) || (0x1d < local_c[(short)uVar4])) goto LAB_004b1699;
      switch(uVar4) {
      default:
        local_1c = 0x40490fdb;
        local_4 = (float)(*psVar5 + 8);
        local_8 = 320.0;
        break;
      case 1:
        iVar2 = *(short *)(iVar1 + 0x314) + 8;
        local_1c = 0x3fc90fdb;
        goto LAB_004b15cf;
      case 2:
        local_1c = 0;
        local_4 = (float)(0x1d8 - *(short *)(iVar1 + 0x312));
        local_8 = 320.0;
        break;
      case 3:
        iVar2 = 0x278 - *(short *)(iVar1 + 0x316);
        local_1c = 0x4096cbe4;
LAB_004b15cf:
        local_8 = (float)iVar2;
        local_4 = 240.0;
      }
      local_8 = local_8 - (float)(int)DAT_007c3140._2_2_;
      local_4 = local_4 - (float)(int)(short)DAT_007c3140;
      if (*(short *)(DAT_0087a478 + 0xc) < 2) {
        uVar3 = (uint)*(ushort *)(iVar1 + 0x348);
      }
      else {
        uVar3 = CONCAT22((short)DAT_007c3140 >> 0xf,*(undefined2 *)(iVar1 + 0x34a));
      }
      local_18 = 0;
      local_10 = 0;
      FUN_004ab8d0(*(undefined4 *)(iVar1 + 0x344),uVar3,&local_18,&local_10);
      if ((local_18 != 0) && (iVar2 = texture_cache_get(0,1), iVar2 != 0)) {
        local_14 = __ftol();
        local_12 = __ftol();
        FUN_004acbb0(4,&local_14,0x3f800000,local_1c,*(undefined4 *)(iVar1 + 0x34c));
      }
LAB_004b1699:
      uVar4 = uVar4 + 1;
    } while (uVar4 < 4);
  }
  return;
}

Disassembly (objdump, 0x4b14c0..0x4b16c7), which is what actually pins register convention
and the field-offset / stack-argument chain used above:

004b14c0:
  83 ec 1c             sub    esp,0x1c
  57                   push   edi
  8b f8                mov    edi,eax                  ; edi = local_player_index (EAX)
  66 83 ff ff          cmp    di,0xffff
  0f 84 d7 01 00 00    je     0x4b16a7                  ; == -1: return
  66 83 ff 01          cmp    di,0x1
  7d 12                jge    0x4b14e8                  ; >= 1: no such local player slot
  8b 0d 78 a4 87 00    mov    ecx,ds:0x87a478            ; local_player_globals
  0f bf c7             movsx  eax,di
  8b 44 81 04          mov    eax,[ecx+eax*4+0x4]        ; local_players[local_player_index]
  83 f8 ff             cmp    eax,0xffffffff
  75 05                jne    0x4b14ed
  4b14e8:
  83 c8 ff             or     eax,0xffffffff             ; treat as -1
  eb 15                jmp    0x4b1502
  4b14ed:
  8b 15 80 a4 87 00    mov    edx,ds:0x87a480             ; player_data
  8b 4a 34             mov    ecx,[edx+0x34]              ; player_data->data
  25 ff ff 00 00       and    eax,0xffff
  c1 e0 09             shl    eax,0x9                     ; * sizeof(player) (0x200)
  8b 44 08 34          mov    eax,[eax+ecx+0x34]          ; player->unit
  4b1502:
  53                   push   ebx
  6a 03                push   0x3
  8b c8                mov    ecx,eax                     ; ECX = unit datum_index
  e8 b4 59 04 00       call   0x4f6ec0                    ; object_try_and_get(unit, 3)
  33 db                xor    ebx,ebx
  83 c4 04             add    esp,0x4
  3b c3                cmp    eax,ebx
  0f 84 93 01 00 00    je     0x4b16ac                    ; no unit: clear the 4 alpha bytes
  55                   push   ebp
  56                   push   esi
  8b 35 1c 94 71 00    mov    esi,ds:0x71941c             ; hud_globals_tag_data
  8d 54 24 20          lea    edx,[esp+0x20]              ; &previous_indicators
  8b c7                mov    eax,edi                     ; EAX = local_player_index
  81 c6 10 03 00 00    add    esi,0x310
  e8 ee 5c fa ff       call   0x457220                    ; player_effect_fade_damage_indicators
  33 ed                xor    ebp,ebp                     ; direction = 0
  4b1534:
  0f bf cd             movsx  ecx,bp
  8a 44 0c 20          mov    al,[esp+ecx*1+0x20]         ; previous_bytes[direction]
  3a c3                cmp    al,bl
  0f 86 56 01 00 00    jbe    0x4b1699                    ; == 0: skip
  3c 1e                cmp    al,0x1e
  0f 83 4e 01 00 00    jae    0x4b1699                    ; >= 0x1e: skip
  ff 24 8d c8 16 4b 00 jmp    [ecx*4+0x4b16c8]             ; 4-entry jump table, one per direction
  4b1552: (direction 0)
  0f bf 16             movsx  edx,WORD PTR [esi]           ; edge_offsets[0]
  83 c2 08             add    edx,0x8
  89 54 24 1c          mov    [esp+0x1c],edx
  c7 44 24 10 db0f4940 mov    DWORD PTR [esp+0x10],0x40490fdb  ; pi
  db 44 24 1c          fild   DWORD PTR [esp+0x1c]
  d9 5c 24 28          fstp   DWORD PTR [esp+0x28]         ; y
  d9 05 1c 32 67 00    fld    DWORD PTR ds:0x67321c        ; 320.0f (screen_center_x); carried to the shared tail as the x source
  eb 63                jmp    0x4b15d7
  4b1574: (direction 1)
  0f bf 46 04          movsx  eax,WORD PTR [esi+0x4]       ; edge_offsets[4]
  83 c0 08             add    eax,0x8
  89 44 24 1c          mov    [esp+0x1c],eax
  c7 44 24 10 db0fc93f mov    DWORD PTR [esp+0x10],0x3fc90fdb  ; pi/2
  db 44 24 1c          fild   DWORD PTR [esp+0x1c]           ; carried to the shared tail as the x source
  eb 42                jmp    0x4b15cf
  4b158d: (direction 2)
  0f bf 4e 02          movsx  ecx,WORD PTR [esi+0x2]        ; edge_offsets[2]
  ba d8 01 00 00       mov    edx,0x1d8
  2b d1                sub    edx,ecx
  89 54 24 1c          mov    [esp+0x1c],edx
  c7 44 24 10 00000000 mov    DWORD PTR [esp+0x10],0x0        ; angle 0
  db 44 24 1c          fild   DWORD PTR [esp+0x1c]
  d9 5c 24 28          fstp   DWORD PTR [esp+0x28]           ; y
  d9 05 1c 32 67 00    fld    DWORD PTR ds:0x67321c           ; 320.0f, carried to the tail as x source
  eb 23                jmp    0x4b15d7
  4b15b4: (direction 3)
  0f bf 46 06          movsx  eax,WORD PTR [esi+0x6]         ; edge_offsets[6]
  b9 78 02 00 00       mov    ecx,0x278
  2b c8                sub    ecx,eax
  89 4c 24 1c          mov    [esp+0x1c],ecx
  c7 44 24 10 e4cb9640 mov    DWORD PTR [esp+0x10],0x4096cbe4  ; 3*pi/2
  db 44 24 1c          fild   DWORD PTR [esp+0x1c]              ; carried to the tail as x source
  4b15cf: (shared: direction 1 and 3 land here directly)
  c7 44 24 28 00007043 mov    DWORD PTR [esp+0x28],0x43700000     ; y = 240.0f
  4b15d7: (shared tail, all four directions)
  0f bf 15 42317c00    movsx  edx,WORD PTR ds:0x7c3142         ; render_viewport_left
  0f bf 05 40317c00    movsx  eax,WORD PTR ds:0x7c3140         ; render_viewport_top
  8b 4e 34             mov    ecx,[esi+0x34]                    ; icon_bitmap
  89 54 24 1c          mov    [esp+0x1c],edx
  8b 15 78a48700       mov    edx,ds:0x87a478                    ; local_player_globals
  66 83 7a 0c 01       cmp    WORD PTR [edx+0xc],0x1
  db 44 24 1c          fild   DWORD PTR [esp+0x1c]                ; float(viewport_left)
  89 44 24 1c          mov    [esp+0x1c],eax                       ; overwrite with viewport_top
  d8 e9                fsubr  st,st(1)                              ; ST0 = (case x source) - viewport_left
  d9 5c 24 24          fstp   DWORD PTR [esp+0x24]                   ; x
  db 44 24 1c          fild   DWORD PTR [esp+0x1c]                    ; float(viewport_top)
  d8 6c 24 28          fsubr  DWORD PTR [esp+0x28]                     ; ST0 = (case y source) - viewport_top
  d9 5c 24 28          fstp   DWORD PTR [esp+0x28]                      ; y
  dd d8                fstp   st(0)                                     ; discard the leftover
                                                                          ; 320.0f the direction 0/2 branches pushed
  7e 06                jle    0x4b161b
  66 8b 46 3a          mov    ax,WORD PTR [esi+0x3a]                     ; sequence_index (split screen)
  eb 06                jmp    0x4b1621
  4b161b:
  33 c0                xor    eax,eax
  66 8b 46 38          mov    ax,WORD PTR [esi+0x38]                       ; sequence_index (single player)
  4b1621:
  8d 54 24 1c          lea    edx,[esp+0x1c]                                ; &sprite_rect
  52                   push   edx
  8d 54 24 18          lea    edx,[esp+0x18]                                 ; &bitmap_data
  52                   push   edx
  50                   push   eax                                             ; sequence_index
  51                   push   ecx                                              ; icon_bitmap
  33 c0                xor    eax,eax                                          ; frame_index = 0 (EAX)
  89 5c 24 24          mov    [esp+0x24],ebx                                   ; *bitmap_data = 0
  89 5c 24 2c          mov    [esp+0x2c],ebx                                    ; *sprite_rect = 0
  e8 94a2ffff          call   0x4ab8d0                                          ; hud_meter_resolve_bitmap_frame
  8b 7c 24 24          mov    edi,[esp+0x24]                                    ; edi = bitmap_data
  83 c4 10             add    esp,0x10
  3b fb                cmp    edi,ebx
  74 52                je     0x4b1699                                          ; no bitmap: skip
  6a 01                push   0x1
  53                   push   ebx
  8b c7                mov    eax,edi
  e8 ff2ef9ff          call   0x444550                                          ; texture_cache_get(bitmap, 0, 1)
  83 c4 08             add    esp,0x8
  85 c0                test   eax,eax
  74 41                je     0x4b1699                                          ; not resident: skip
  d9 44 24 24          fld    DWORD PTR [esp+0x24]                              ; x
  e8 537b1800          call   0x6391b4                                          ; __ftol
  d9 44 24 28          fld    DWORD PTR [esp+0x28]                              ; y
  66 89 44 24 18       mov    WORD PTR [esp+0x18],ax                            ; position.x
  e8 457b1800          call   0x6391b4                                          ; __ftol
  8b 4c 24 10          mov    ecx,[esp+0x10]                                     ; rotation_bits
  66 89 44 24 1a       mov    WORD PTR [esp+0x1a],ax                            ; position.y
  8b 46 3c             mov    eax,[esi+0x3c]                                     ; icon_color
  50                   push   eax                                                 ; color
  8b 44 24 20          mov    eax,[esp+0x20]                                      ; EAX = sprite_rect
  51                   push   ecx                                                  ; rotation_bits
  68 0000803f          push   0x3f800000                                           ; scale = 1.0f
  8d 54 24 24          lea    edx,[esp+0x24]                                       ; &position
  52                   push   edx
  6a 04                push   0x4                                                   ; anchor = 4
  32 c9                xor    cl,cl                                                  ; is_sprite_bitmap = 0
  8b d7                mov    edx,edi                                                ; EDX = bitmap_data
  e8 1ab5ffff          call   0x4acbb0
  83 c4 14             add    esp,0x14
  4b1699:
  45                   inc    ebp                                                     ; direction++
  66 83 fd 04          cmp    bp,0x4
  0f 82 90feffff       jb     0x4b1534
  4b16a4:
  5e                   pop    esi
  5d                   pop    ebp
  5b                   pop    ebx
  4b16a7:
  5f                   pop    edi
  83 c4 1c             add    esp,0x1c
  c3                   ret
  4b16ac:
  8b 0d 84186f00       mov    ecx,ds:0x6f1884                                        ; player_effect_globals_pointer
  0f bf c7             movsx  eax,di
  69 c0 ec000000       imul   eax,eax,0xec                                            ; sizeof(player_effect)
  89 9c 08 e4000000    mov    [eax+ecx+0xe4],ebx                                       ; damage_indicator_alpha[4] = 0
  5b                   pop    ebx
  5f                   pop    edi
  83 c4 1c             add    esp,0x1c
  c3                   ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
