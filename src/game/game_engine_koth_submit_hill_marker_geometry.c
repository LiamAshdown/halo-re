// game_engine_koth_submit_hill_marker_geometry  (Ghidra: FUN_0046b2f0; named per this rewrite)
// address 0x46b2f0, size 1008 bytes
// name confidence: 0.3   rewrite confidence: 0.1
// evidence: out/phase4/game_functions.md ("Assembles and submits a small render/decal geometry
//   batch (positions, colors, default hill-marker placement) to draw the moving King-of-the-Hill
//   marker"). This function is almost entirely rasterizer/render-module plumbing (callees
//   0x511e80/0x51bd60/0x51bdd0/0x51be40/0x526f50/0x52b050/0x52b180/0x52b530 and roughly two
//   dozen globals all sit in the 0x50xxxx/0x006dxxxx/0x0065xxxx/0x0069xxxx/0x006exxxx/0x007xxxxx
//   ranges the render module owns), and that module's types have not been recovered yet (PLAN.md
//   places rasterizer/render last, "Direct3D-heavy, least reusable"). This rewrite therefore
//   preserves every operation LITERALLY (renamed globals only where types/game.h or an
//   already-committed module already names them) rather than inventing render-module types or
//   field names; it should be re-reviewed once the render module gets its own type pass.
// reconciled: 0x006851fc is a pointer to the opaque-white ColorARGB (0x00655138); one name global_white_argb: an object/tag handle in EAX (in_EAX, read as a float* -- almost
//   certainly mistyped by Ghidra); param_1..param_5 are this function's own stack parameters.
//   // blam-cc: stack -> tag_handle_as_uint, position_override, orientation_override,
//   //   param_4, param_5; unaff_EAX -> vertex_source
// UNSURE: essentially everything below the immediate control flow -- see evidence. iStack_fc is
//   read without ever being assigned in this function (a genuine Ghidra gap, not introduced
//   here); left as a zero-initialized local to keep the translation well-defined, flagged below.
// reconciled: R77 0x0069c632 uint8 render_koth_marker_active -> int16 rasterizer_vertex_buffer_lock_state (all stores are WORD)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "fn_game.h"

extern int16_t rasterizer_vertex_buffer_lock_state; // 0x0069c632, rasterizer.h; WORD stores (0x46b306, 0x46b6c2, 0x46b6d2)
extern void **rasterizer_dynamic_index_buffer;                // 0x006e09e8, UNSURE identity (vtable object, +0x30 called)
extern int16_t rasterizer_dynamic_vertex_slots[];      // 0x006d99d8, UNSURE identity
extern int32_t render_unknown_d98f0[];      // 0x006d98f0, UNSURE identity
extern uint8_t render_unknown_7bf04c[];     // 0x007bf04c, UNSURE identity
extern tag_instance *tag_instances;         // 0x0087bc14
extern void *k_render_identity_matrix_ptr;              // 0x0069673c, UNSURE identity
extern real_vector3d global_white_color;  // 0x00686b04, UNSURE: reused as a generic 3-float default
extern const ColorARGB *global_white_argb;     // 0x006851fc, UNSURE identity (16 bytes copied)
extern real_vector3d default_axis_b;        // 0x00686b0c, UNSURE identity
extern king_hill_marker_history king_hill_markers; // 0x0087a9a0
extern uint8_t console_debug_toggle_6893ec;          // 0x006893ec, UNSURE identity
extern uint8_t rasterizer_render_states_dirty;          // 0x0069c74c, UNSURE identity
extern uint8_t unknown_0071d1fa;          // 0x0071d1fa, UNSURE identity
extern uint32_t rasterizer_device_version;      // 0x007c118c, UNSURE identity
extern void **rasterizer_device;        // 0x0071d174, UNSURE identity (vtable object, +0xe4 called)

extern void *rasterizer_dynamic_index_cache_reserve(void); // 0x51bd60, render module, UNSURE signature
extern int32_t rasterizer_dynamic_vertex_cache_reserve(void); // 0x51bdd0, render module, UNSURE signature
extern int32_t rasterizer_dynamic_vertex_cache_lock(void); // 0x51be40, render module, UNSURE signature
extern void *rasterizer_dynamic_index_slot_lock(void); // 0x511e80, render module, UNSURE signature
extern void rasterizer_model_draw_prepare_states(int32_t a); // 0x526f50, render module, UNSURE signature
extern void rasterizer_shader_environment_draw_dispatch(int32_t tag_data, int32_t a, int32_t b, int32_t c, int32_t d, int32_t e); // 0x52b050
extern void rasterizer_transparent_geometry_group_build(int32_t tag_data, int32_t a, int32_t b, int32_t c, int32_t d, int32_t e,
    int32_t f, void *g); // 0x52b180
extern void rasterizer_model_draw_restore_states(void); // 0x52b530

// blam-cc: stack -> tag_handle_as_uint, position_override, orientation_override, param_4, param_5;
//   unaff_EAX -> vertex_source
// UNSURE: see header -- a near-literal transcription of the render-module plumbing.
void game_engine_koth_submit_hill_marker_geometry(uint32_t tag_handle_as_uint,
    uint32_t *position_override, uint32_t *orientation_override, uint32_t param_4,
    uint32_t param_5, float *vertex_source)
{
    float fVar5;
    int32_t iVar6;
    float local_f0;
    float fStack_ec;
    float fStack_e8;
    int32_t iStack_fc = 0; // UNSURE: never assigned in the original either; see header

    rasterizer_vertex_buffer_lock_state = 9;
    fVar5 = *(float *)rasterizer_dynamic_index_cache_reserve();
    local_f0 = fVar5;
    iVar6 = rasterizer_dynamic_vertex_cache_reserve();
    if (fVar5 == fVar5 && iVar6 != -1) { // "fVar5 != -NAN" -- i.e. fVar5 is not NaN
        uint8_t *dest_block;
        int32_t base;
        int32_t offset;
        float *src;
        uint32_t *dst;
        int32_t count;
        int32_t tag_data;
        int32_t i;

        base = rasterizer_dynamic_vertex_cache_lock();
        dest_block = (uint8_t *)rasterizer_dynamic_index_slot_lock();
        offset = (int32_t)vertex_source - base;
        src = vertex_source + 9;
        dst = (uint32_t *)(base + 0xc);
        count = 4;
        do {
            dst[-3] = ((uint32_t *)src)[-9];
            dst[-2] = ((uint32_t *)src)[-8];
            dst[-1] = ((uint32_t *)src)[-7];
            {
                uint32_t *from = (uint32_t *)(offset + (int32_t)dst);
                dst[0] = from[0];
                dst[1] = from[1];
                dst[2] = from[2];
            }
            dst[3] = ((uint32_t *)src)[-3];
            dst[4] = ((uint32_t *)src)[-2];
            dst[5] = ((uint32_t *)src)[-1];
            dst[6] = ((uint32_t *)src)[0];
            dst[7] = ((uint32_t *)src)[1];
            dst[8] = ((uint32_t *)src)[2];
            dst[9] = ((uint32_t *)src)[3];
            dst[10] = ((uint32_t *)src)[4];
            *(uint16_t *)(dst + 0xb) = *(uint16_t *)(src + 5);
            *(uint16_t *)((uint8_t *)dst + 0x2e) = *(uint16_t *)((uint8_t *)src + 0x16);
            dst[0xc] = 0x3f000000;
            dst[0xd] = 0x3f000000;
            src = src + 0x11;
            dst = dst + 0x11;
            count--;
        } while (count != 0);

        *(uint16_t *)dest_block = 0;
        *(uint16_t *)(dest_block + 2) = 1;
        *(uint16_t *)(dest_block + 4) = 2;
        *(uint16_t *)(dest_block + 6) = 2;
        *(uint16_t *)(dest_block + 8) = 3;
        *(uint16_t *)(dest_block + 10) = 0;

        ((void (__stdcall *)(void **))(*(void ***)((uint8_t *)*rasterizer_dynamic_index_buffer + 0x30)))(rasterizer_dynamic_index_buffer); // TYPES-GAP vtable call

        {
            int16_t sub_index = rasterizer_dynamic_vertex_slots[iStack_fc * 8]; // *0x10 bytes / 2 == *8 int16 units
            if (render_unknown_d98f0[sub_index * 3] != 0) {
                void **sub = (void **)(render_unknown_7bf04c + render_unknown_d98f0[sub_index * 3] * 10);
                ((void (__stdcall *)(void **))(*(void ***)((uint8_t *)*sub + 0x30)))(sub); // TYPES-GAP vtable call
            }
        }

        tag_data = *(int32_t *)((uint8_t *)tag_instances[tag_handle_as_uint & 0xffff].data + 0);
        local_f0 = (vertex_source[0x33] + vertex_source[0x22] + vertex_source[0x11] + vertex_source[0]) * 0.25f;
        fStack_ec = (vertex_source[0x34] + vertex_source[0x23] + vertex_source[0x12] + vertex_source[1]) * 0.25f;
        {
            float fVar1 = vertex_source[0x35];
            float fVar2 = vertex_source[0x24];
            float fVar3 = vertex_source[0x13];
            float fVar4 = vertex_source[2];
            fStack_e8 = (fVar1 + fVar2 + fVar3 + fVar4) * 0.25f;
        }

        {
            // UNSURE: the raw ~0xd8-byte scratch record submitted below is left as an opaque
            // byte buffer -- see header. Ghidra's own field-by-field zero/fill is preserved as
            // a memset followed by the literal writes it shows.
            uint8_t record[0xd8];
            for (i = 0; i < (int32_t)sizeof(record); i++) record[i] = 0;

            *(uint32_t *)(record + 4) = 1;
            *(uint16_t *)(record + 8) = 1;
            *(void **)(record + 0xa) = k_render_identity_matrix_ptr;

            if (position_override == (uint32_t *)0) {
                *(real_vector3d *)(record + 0xe) = global_white_color;
                *(uint16_t *)(record + 0x1a) = 0;
                *(uint16_t *)(record + 0x50) = 0;
                for (i = 0; i < 16; i++) {
                    (record + 0x54)[i] = ((const uint8_t *)global_white_argb)[i];
                }
                *(real_vector3d *)(record + 0x64) = default_axis_b;
                *(uint32_t *)(record + 0x70) = 0;
                *(uint32_t *)(record + 0x74) = 0x3f800000;
                *(uint32_t *)(record + 0x78) = 0;
            } else {
                uint32_t *dstp = (uint32_t *)(record + 0xe);
                uint32_t *srcp = position_override;
                for (i = 0; i < 0x1d; i++) {
                    dstp[i] = srcp[i];
                }
            }

            if (orientation_override == (uint32_t *)0) {
                *(void **)(record + 0xc8) = &king_hill_markers.position[0];
                *(void **)(record + 0xcc) = &king_hill_markers.state[0];
            } else {
                *(void **)(record + 0xc8) = (void *)orientation_override[0];
                *(void **)(record + 0xcc) = (void *)orientation_override[1];
            }
            *(uint32_t *)(record + 0xd0) = param_4;
            *(uint32_t *)(record + 0xd4) = param_5;
            *(float *)(record + 0x98) = local_f0;
            *(float *)(record + 0x9c) = fStack_ec;
            *(float *)(record + 0xa0) = fStack_e8;

            if (console_debug_toggle_6893ec != 0) {
                rasterizer_render_states_dirty = 1;
                unknown_0071d1fa = 0;
                if (rasterizer_device_version < 0xffff0101) {
                    ((void (__stdcall *)(void **, int32_t, int32_t))(*(void ***)((uint8_t *)*rasterizer_device + 0xe4)))(
                        rasterizer_device, 0x89, 1); // TYPES-GAP vtable call
                }
            }

            rasterizer_model_draw_prepare_states(1);

            {
                int16_t kind = *(int16_t *)((uint8_t *)tag_data + 0x24);
                if (kind == 1 || (4 < kind && kind < 0xc)) {
                    rasterizer_transparent_geometry_group_build(tag_data, 0, 0, 0, 2, 0, iStack_fc, &local_f0);
                } else {
                    rasterizer_shader_environment_draw_dispatch(tag_data, 0, 0, 0, 2, 0);
                }
            }
            rasterizer_model_draw_restore_states();

            if (console_debug_toggle_6893ec != 0 && rasterizer_device_version < 0xffff0101) {
                ((void (__stdcall *)(void **, int32_t, int32_t))(*(void ***)((uint8_t *)*rasterizer_device + 0xe4)))(
                    rasterizer_device, 0x89, 0); // TYPES-GAP vtable call
            }
        }

        rasterizer_vertex_buffer_lock_state = 0;
        return;
    }
    rasterizer_vertex_buffer_lock_state = 0;
}

#if 0
Original Ghidra decompilation (0x46b2f0), from tools/pack.py 0x46b2f0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0046b2f0(uint param_1,undefined4 *param_2,undefined4 *param_3,undefined4 param_4,
                 undefined4 param_5)

{
  float fVar1;
  float fVar2;
  float fVar3;
  short sVar4;
  float *in_EAX;
  float fVar5;
  int iVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;
  int iVar9;
  float *pfVar10;
  undefined **ppuVar11;
  int iStack_fc;
  int local_f4;
  float local_f0;
  float fStack_ec;
  float fStack_e8;
  int local_e0;
  undefined *local_dc [3];
  undefined2 uStack_d0;
  undefined4 auStack_cc [3];
  undefined2 uStack_c0;
  undefined2 uStack_8c;
  undefined4 uStack_80;
  undefined4 uStack_7c;
  undefined4 uStack_78;
  undefined4 uStack_74;
  undefined4 uStack_70;
  undefined4 uStack_6c;
  undefined4 uStack_68;
  undefined4 uStack_64;
  undefined4 uStack_60;
  undefined4 uStack_5c;
  undefined *puStack_58;
  undefined *puStack_54;
  float fStack_28;
  float fStack_24;
  float fStack_20;
  undefined4 uStack_18;
  undefined4 uStack_14;

  _DAT_0069c632 = 9;
  fVar5 = (float)FUN_0051bd60();
  local_f0 = fVar5;
  iVar6 = FUN_0051bdd0();
  if ((fVar5 != -NAN) && (iVar6 != -1)) {
    iVar6 = FUN_0051be40();
    local_dc[0] = (undefined *)FUN_00511e80();
    local_e0 = (int)in_EAX - iVar6;
    pfVar10 = in_EAX + 9;
    puVar8 = (undefined4 *)(iVar6 + 0xc);
    local_f4 = 4;
    do {
      puVar8[-3] = pfVar10[-9];
      puVar8[-2] = pfVar10[-8];
      puVar8[-1] = pfVar10[-7];
      puVar7 = (undefined4 *)(local_e0 + (int)puVar8);
      *puVar8 = *puVar7;
      puVar8[1] = puVar7[1];
      puVar8[2] = puVar7[2];
      puVar8[3] = pfVar10[-3];
      puVar8[4] = pfVar10[-2];
      puVar8[5] = pfVar10[-1];
      puVar8[6] = *pfVar10;
      puVar8[7] = pfVar10[1];
      puVar8[8] = pfVar10[2];
      puVar8[9] = pfVar10[3];
      puVar8[10] = pfVar10[4];
      *(undefined2 *)(puVar8 + 0xb) = *(undefined2 *)(pfVar10 + 5);
      *(undefined2 *)((int)puVar8 + 0x2e) = *(undefined2 *)((int)pfVar10 + 0x16);
      puVar8[0xc] = 0x3f000000;
      puVar8[0xd] = 0x3f000000;
      pfVar10 = pfVar10 + 0x11;
      puVar8 = puVar8 + 0x11;
      local_f4 = local_f4 + -1;
    } while (local_f4 != 0);
    *(undefined2 *)local_dc[0] = 0;
    *(undefined2 *)((int)local_dc[0] + 2) = 1;
    *(undefined2 *)((int)local_dc[0] + 4) = 2;
    *(undefined2 *)((int)local_dc[0] + 6) = 2;
    *(undefined2 *)((int)local_dc[0] + 8) = 3;
    *(undefined2 *)((int)local_dc[0] + 10) = 0;
    (**(code **)(*DAT_006e09e8 + 0x30))(DAT_006e09e8);
    if ((&DAT_006d98f0)[*(short *)(&DAT_006d99d8 + iStack_fc * 0x10) * 3] != 0) {
      (**(code **)(**(int **)(&DAT_007bf04c +
                             (&DAT_006d98f0)[*(short *)(&DAT_006d99d8 + iStack_fc * 0x10) * 3] * 10)
                  + 0x30))
                (*(int **)(&DAT_007bf04c +
                          (&DAT_006d98f0)[*(short *)(&DAT_006d99d8 + iStack_fc * 0x10) * 3] * 10));
    }
    iVar6 = *(int *)((param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    local_f0 = (in_EAX[0x33] + in_EAX[0x22] + in_EAX[0x11] + *in_EAX) * 0.25;
    fStack_ec = (in_EAX[0x34] + in_EAX[0x23] + in_EAX[0x12] + in_EAX[1]) * 0.25;
    fVar5 = in_EAX[0x35];
    fVar1 = in_EAX[0x24];
    fVar2 = in_EAX[0x13];
    fVar3 = in_EAX[2];
    ppuVar11 = local_dc;
    for (iVar9 = 0x33; iVar9 != 0; iVar9 = iVar9 + -1) {
      *ppuVar11 = (undefined *)0x0;
      ppuVar11 = ppuVar11 + 1;
    }
    fStack_e8 = (fVar5 + fVar1 + fVar2 + fVar3) * 0.25;
    local_dc[1] = (undefined *)0x1;
    uStack_d0 = 1;
    local_dc[2] = PTR_DAT_0069673c;
    if (param_2 == (undefined4 *)0x0) {
      auStack_cc[0] = *(undefined4 *)PTR_DAT_00686b04;
      auStack_cc[1] = *(undefined4 *)(PTR_DAT_00686b04 + 4);
      auStack_cc[2] = *(undefined4 *)(PTR_DAT_00686b04 + 8);
      uStack_c0 = 0;
      uStack_8c = 0;
      uStack_80 = *(undefined4 *)PTR_DAT_006851fc;
      uStack_7c = *(undefined4 *)(PTR_DAT_006851fc + 4);
      uStack_78 = *(undefined4 *)(PTR_DAT_006851fc + 8);
      uStack_74 = *(undefined4 *)(PTR_DAT_006851fc + 0xc);
      uStack_64 = *(undefined4 *)PTR_DAT_00686b0c;
      uStack_60 = *(undefined4 *)(PTR_DAT_00686b0c + 4);
      uStack_5c = *(undefined4 *)(PTR_DAT_00686b0c + 8);
      uStack_70 = 0;
      uStack_6c = 0x3f800000;
      uStack_68 = 0;
    }
    else {
      puVar8 = auStack_cc;
      for (iVar9 = 0x1d; iVar9 != 0; iVar9 = iVar9 + -1) {
        *puVar8 = *param_2;
        param_2 = param_2 + 1;
        puVar8 = puVar8 + 1;
      }
    }
    if (param_3 == (undefined4 *)0x0) {
      puStack_58 = &DAT_0087a9a0;
      puStack_54 = &DAT_0087a9d0;
    }
    else {
      puStack_58 = (undefined *)*param_3;
      puStack_54 = (undefined *)param_3[1];
    }
    uStack_18 = param_4;
    uStack_14 = param_5;
    fStack_28 = local_f0;
    fStack_24 = fStack_ec;
    fStack_20 = fStack_e8;
    if (DAT_006893ec != '\0') {
      DAT_0069c74c = 1;
      DAT_0071d1fa = 0;
      if (DAT_007c118c < 0xffff0101) {
        (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x89,1);
      }
    }
    FUN_00526f50(1);
    sVar4 = *(short *)(iVar6 + 0x24);
    if ((sVar4 == 1) || ((4 < sVar4 && (sVar4 < 0xc)))) {
      FUN_0052b180(iVar6,0,0,0,2,0,iStack_fc,&local_f0);
    }
    else {
      FUN_0052b050(iVar6,0,0,0,2,0);
    }
    FUN_0052b530();
    if ((DAT_006893ec != '\0') && (DAT_007c118c < 0xffff0101)) {
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x89,0);
    }
    _DAT_0069c632 = 0;
    return;
  }
  _DAT_0069c632 = 0;
  return;
}
#endif
