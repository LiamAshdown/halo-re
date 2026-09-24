// rasterizer_end_frame  (Ghidra: chimera__rasterizer_globals; Chimera's rasterizer_globals_sig and
// text_hook_sig both anchor here, which is where the mechanical name came from)
// address 0x517b90, size 1339 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: ends with IDirect3DDevice9::EndScene (+0xa8) and clears 0x0071d16f, the flag that
//   rasterizer_reset_device_if_needed 0x517500 sets after BeginScene (+0xa4); called from the
//   render module after every frame (0x50bf9c, 0x50c64b), from 0x4921d7 and from the frame
//   start 0x5173d5 right after 0x517500 succeeds. Before that it composites render target 1
//   (0x0069d37c = rasterizer_render_targets[1].texture) onto the back buffer surface
//   (0x0069d364 = rasterizer_render_targets[0].surface) as a pre-transformed quad
//   (FVF 0x144 XYZRHW|DIFFUSE|TEX1, four vertices, DrawIndexedPrimitive fan), draws the chat
//   bar backdrop when the chat dialog is open, restores the default blend states and runs the
//   UI overlay render callback. The whole body is decoded from the raw code: Ghidra lost the
//   viewport block, the lock pointer and the EndScene success latch.
//   The summary in out/phase4/rasterizer_functions.md (cinematic letterbox) is wrong; so was
//   the header note calling 0x0069c630 the letterbox flag. Its only writer is the frame start
//   at 0x517421, which sets it to 1.
// register convention: none, __cdecl with no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_device;                                     // 0x0071d174
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern rasterizer_render_target rasterizer_render_targets[k_rasterizer_render_targets]; // 0x0069d358
extern int16_t rasterizer_active_render_target;                     // 0x0069d350
extern void *rasterizer_render_target_vertex_buffer;                // 0x0071d20c
extern void *rasterizer_render_target_index_buffer;                 // 0x0071d208
extern uint8_t rasterizer_software_vertex_processing;               // 0x0069c680
extern uint8_t rasterizer_frame_started;                            // 0x0069c630 UNSURE name
extern uint8_t rasterizer_caps_flag_68a;                            // 0x0069c68a
extern uint8_t rasterizer_in_scene;                                 // 0x0071d16f set after BeginScene, cleared after EndScene
extern uint8_t console_debug_toggle_6893e6;                         // 0x006893e6 wireframe
extern uint32_t rasterizer_render_target_unnormalized_uvs;          // 0x00722b28 UNSURE: nonzero
                                                                    //   scales the quad UVs to texels
extern uint8_t chat_dialog_open;                                    // 0x006b3858
extern void *chat_gui_root_handle;                                  // 0x00721ea4 KSML UI engine instance (interface module name)
extern int32_t (*unknown_00721ebc)(void *engine);                   // 0x00721ebc UNSURE: UI overlay render
extern int32_t rasterizer_ui_render_failed;                         // 0x0071d168 UNSURE name

// blam-cc: EAX packed_color, ECX rect
extern void ui_draw_filled_rectangle(uint32_t packed_color, Rectangle2D *rect); // 0x449780, solid rectangle fill
extern uint32_t __stdcall D3DXGetFVFVertexSize(uint32_t fvf); // 0x00583b17

typedef int32_t (__stdcall *d3d_call1_fn)(void *self, uint32_t a);
typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);
typedef int32_t (__stdcall *d3d_call3_fn)(void *self, uint32_t a, uint32_t b, uint32_t c);
typedef int32_t (__stdcall *d3d_set_render_target_fn)(void *self, uint32_t index, void *surface);
typedef int32_t (__stdcall *d3d_get_desc_fn)(void *self, void *desc);
typedef int32_t (__stdcall *d3d_set_viewport_fn)(void *self, const void *viewport);
typedef int32_t (__stdcall *d3d_set_texture_fn)(void *self, uint32_t stage, void *texture);
typedef int32_t (__stdcall *d3d_lock_fn)(void *self, uint32_t offset, uint32_t size, void **data, uint32_t flags);
typedef int32_t (__stdcall *d3d_set_stream_source_fn)(void *self, uint32_t stream, void *buffer, uint32_t offset, uint32_t stride);
typedef int32_t (__stdcall *d3d_set_indices_fn)(void *self, void *index_buffer);
typedef int32_t (__stdcall *d3d_draw_indexed_primitive_fn)(void *self, uint32_t type, int32_t base_vertex, uint32_t min_index,
                                                 uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count);
typedef int32_t (__stdcall *d3d_call0_fn)(void *self);




static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

static void rasterizer_set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)device_vtable()[0xe4 / 4])(rasterizer_device, state, value);
}

static void rasterizer_set_sampler_state(uint32_t stage, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x114 / 4])(rasterizer_device, stage, type, value);
}

static void rasterizer_set_texture_stage_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    ((d3d_call3_fn)device_vtable()[0x10c / 4])(rasterizer_device, sampler, type, value);
}

static void rasterizer_set_quad_vertex(rasterizer_screen_vertex *vertex, float x, float y, float u, float v)
{
    vertex->x = x;
    vertex->y = y;
    vertex->z = 0.0f;
    vertex->rhw = 1.0f;
    vertex->diffuse = 0xffffffff;
    vertex->u = u;
    vertex->v = v;
}

void rasterizer_end_frame(void)
{
    uint8_t succeeded = 1;

    if (rasterizer_frame_started == 1 && rasterizer_caps_flag_68a == 0) {
        void *back_buffer = (void *)rasterizer_render_targets[0].surface;
        int16_t width = (int16_t)(rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left);
        int16_t height = (int16_t)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top);
        d3d_surface_desc desc;
        d3d_viewport viewport;
        rasterizer_screen_vertex *vertices = 0;
        uint32_t stride;

        // back to the back buffer, full surface viewport
        ((d3d_set_render_target_fn)device_vtable()[0x94 / 4])(rasterizer_device, 0, back_buffer);
        rasterizer_active_render_target = 0;
        ((d3d_get_desc_fn)(*(void ***)back_buffer)[0x30 / 4])(back_buffer, &desc);
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = desc.width;
        viewport.height = desc.height;
        viewport.min_z = 0.0f;
        viewport.max_z = 1.0f;
        ((d3d_set_viewport_fn)device_vtable()[0xbc / 4])(rasterizer_device, &viewport);

        if (console_debug_toggle_6893e6 != 0) {
            rasterizer_set_render_state(8, 3);                  // D3DRS_FILLMODE solid for the blit
        }

        stride = D3DXGetFVFVertexSize(0x144);
        ((d3d_call1_fn)device_vtable()[0x1ac / 4])(rasterizer_device, 0);  // SetPixelShader(NULL)
        ((d3d_set_texture_fn)device_vtable()[0x104 / 4])(rasterizer_device, 0,
                                                         (void *)rasterizer_render_targets[1].texture);
        rasterizer_set_sampler_state(0, 1, 3);
        rasterizer_set_sampler_state(0, 2, 3);
        rasterizer_set_sampler_state(0, 5, 1);
        rasterizer_set_sampler_state(0, 6, 1);
        rasterizer_set_sampler_state(0, 7, 1);
        rasterizer_set_render_state(0x16, 3);                   // D3DRS_CULLMODE
        rasterizer_set_render_state(0xa8, 7);                   // D3DRS_COLORWRITEENABLE rgb
        rasterizer_set_render_state(0x1b, 0);                   // D3DRS_ALPHABLENDENABLE
        rasterizer_set_render_state(0xf, 0);                    // D3DRS_ALPHATESTENABLE
        rasterizer_set_render_state(7, 0);                      // D3DRS_ZENABLE
        rasterizer_set_render_state(0x1c, 0);                   // D3DRS_FOGENABLE
        rasterizer_set_texture_stage_state(0, 1, 2);            // COLOROP selectarg1
        rasterizer_set_texture_stage_state(0, 2, 2);            // COLORARG1 texture
        rasterizer_set_texture_stage_state(0, 4, 2);
        rasterizer_set_texture_stage_state(0, 5, 2);
        rasterizer_set_texture_stage_state(1, 1, 1);
        rasterizer_set_texture_stage_state(1, 4, 1);

        if (rasterizer_render_target_vertex_buffer != 0) {
            void *buffer = rasterizer_render_target_vertex_buffer;

            ((d3d_lock_fn)(*(void ***)buffer)[0x2c / 4])(buffer, 0, stride * 4, (void **)&vertices, 0x2000);
            if (vertices != 0) {
                float right = (float)width - 0.5f;
                float bottom = (float)height - 0.5f;

                rasterizer_set_quad_vertex(&vertices[0], -0.5f, -0.5f, 0.0f, 0.0f);
                rasterizer_set_quad_vertex(&vertices[1], right, -0.5f, 1.0f, 0.0f);
                rasterizer_set_quad_vertex(&vertices[2], right, bottom, 1.0f, 1.0f);
                rasterizer_set_quad_vertex(&vertices[3], -0.5f, bottom, 0.0f, 1.0f);
                if (rasterizer_render_target_unnormalized_uvs != 0) {
                    vertices[1].u *= (float)width;
                    vertices[2].u *= (float)width;
                    vertices[2].v *= (float)height;
                    vertices[3].v *= (float)height;
                }

                ((d3d_call1_fn)device_vtable()[0x170 / 4])(rasterizer_device, 0);      // SetVertexShader(NULL)
                ((d3d_call1_fn)device_vtable()[0x164 / 4])(rasterizer_device, 0x144);  // SetFVF
                ((d3d_call0_fn)(*(void ***)buffer)[0x30 / 4])(buffer);                 // Unlock
                ((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing);
                ((d3d_set_stream_source_fn)device_vtable()[0x190 / 4])(rasterizer_device, 0,
                                                                       rasterizer_render_target_vertex_buffer, 0, stride);
                ((d3d_set_indices_fn)device_vtable()[0x1a0 / 4])(rasterizer_device, rasterizer_render_target_index_buffer);
                ((d3d_draw_indexed_primitive_fn)device_vtable()[0x148 / 4])(rasterizer_device, 6, 0, 0, 4, 0, 2);
                ((d3d_call1_fn)device_vtable()[0x164 / 4])(rasterizer_device, 0);      // SetFVF(0)
            }
        }

        if (console_debug_toggle_6893e6 != 0) {
            rasterizer_set_render_state(8, 2);                  // back to wireframe
        }
    }

    if (chat_dialog_open != 0) {
        Rectangle2D chat_bar;

        chat_bar.top = 0x1cc;
        chat_bar.left = 0;
        chat_bar.bottom = 0x1e0;
        chat_bar.right = 0x280;
        ui_draw_filled_rectangle(0xb0202020, &chat_bar);
    }

    if (((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing) < 0) {
        succeeded = 0;
    }
    rasterizer_set_render_state(0x13, 5);                       // D3DRS_SRCBLEND srcalpha
    rasterizer_set_render_state(0x14, 6);                       // D3DRS_DESTBLEND invsrcalpha
    rasterizer_set_render_state(0xab, 1);                       // D3DRS_BLENDOP add

    if (chat_gui_root_handle != 0 && rasterizer_ui_render_failed == 0 &&
        unknown_00721ebc(chat_gui_root_handle) < 0) {
        rasterizer_ui_render_failed = 1;
    }

    if (((d3d_call1_fn)device_vtable()[0x134 / 4])(rasterizer_device, rasterizer_software_vertex_processing) < 0) {
        succeeded = 0;
    }
    if (((d3d_call0_fn)device_vtable()[0xa8 / 4])(rasterizer_device) >= 0 && succeeded != 0) {  // EndScene
        rasterizer_in_scene = 0;
    }
}

#if 0
Original Ghidra decompilation (0x517b90):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void chimera__rasterizer_globals(void)

{
  float fVar1;
  short sVar2;
  short sVar3;
  int iVar4;
  short sVar5;
  int *piVar6;
  undefined4 *puStack_44;
  float local_40;
  undefined4 local_3c;
  undefined4 uStack_38;
  undefined1 auStack_2c [16];
  undefined4 *puStack_1c;
  float fStack_18;
  char cVar7;
  
  piVar6 = DAT_0069d364;
  if ((DAT_0069c630 == '\x01') && (DAT_0069c68a == '\0')) {
    sVar3 = (short)DAT_007c1254;
    sVar2 = (short)DAT_007c1258;
    sVar5 = DAT_007c1258._2_2_ - DAT_007c1254._2_2_;
    (**(code **)(*DAT_0071d174 + 0x94))(DAT_0071d174,0,DAT_0069d364);
    _DAT_0069d350 = 0;
    (**(code **)(*piVar6 + 0x30))(piVar6,auStack_2c);
    local_40 = fStack_18;
    puStack_44 = puStack_1c;
    local_3c = 0;
    uStack_38 = 0x3f800000;
    (**(code **)(*DAT_0071d174 + 0xbc))(DAT_0071d174,&stack0xffffffb4);
    if (DAT_006893e6 != '\0') {
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,8,3);
    }
    iVar4 = FUN_00583b17(0x144);
    (**(code **)(*DAT_0071d174 + 0x1ac))(DAT_0071d174,0);
    (**(code **)(*DAT_0071d174 + 0x104))(DAT_0071d174,0,DAT_0069d37c);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,1,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,2,3);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,5,1);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,6,1);
    (**(code **)(*DAT_0071d174 + 0x114))(DAT_0071d174,0,7,1);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x16,3);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xa8,7);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1b,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xf,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,7,0);
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,1,2);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,2,2);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,4,2);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,0,5,2);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,1,1);
    (**(code **)(*DAT_0071d174 + 0x10c))(DAT_0071d174,1,4,1);
    if ((DAT_0071d20c != (int *)0x0) &&
       ((**(code **)(*DAT_0071d20c + 0x2c))(DAT_0071d20c,0,iVar4 * 4,&puStack_44,0x2000),
       puStack_44 != (undefined4 *)0x0)) {
      puStack_44[5] = 0;
      puStack_44[6] = 0;
      *puStack_44 = 0xbf000000;
      puStack_44[1] = 0xbf000000;
      puStack_44[0xc] = 0x3f800000;
      puStack_44[0xd] = 0;
      fVar1 = (float)(int)sVar5;
      puStack_44[7] = fVar1 - 0.5;
      puStack_44[8] = 0xbf000000;
      puStack_44[0x13] = 0x3f800000;
      puStack_44[0x14] = 0x3f800000;
      puStack_44[0xe] = fVar1 - 0.5;
      local_40 = (float)(int)(short)(sVar2 - sVar3);
      puStack_44[0xf] = local_40 - 0.5;
      puStack_44[0x1a] = 0;
      puStack_44[0x1b] = 0x3f800000;
      puStack_44[0x15] = 0xbf000000;
      puStack_44[0x16] = local_40 - 0.5;
      puStack_44[0x19] = 0xffffffff;
      puStack_44[0x12] = 0xffffffff;
      puStack_44[0xb] = 0xffffffff;
      puStack_44[4] = 0xffffffff;
      puStack_44[0x17] = 0;
      puStack_44[0x10] = 0;
      puStack_44[9] = 0;
      puStack_44[2] = 0;
      puStack_44[0x18] = 0x3f800000;
      puStack_44[0x11] = 0x3f800000;
      puStack_44[10] = 0x3f800000;
      puStack_44[3] = 0x3f800000;
      if (DAT_00722b28 != 0) {
        puStack_44[0xc] = fVar1 * (float)puStack_44[0xc];
        puStack_44[0x13] = fVar1 * (float)puStack_44[0x13];
        puStack_44[0x14] = local_40 * (float)puStack_44[0x14];
        puStack_44[0x1b] = local_40 * (float)puStack_44[0x1b];
      }
      (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174,0);
      (**(code **)(*DAT_0071d174 + 0x164))(DAT_0071d174,0x144);
      (**(code **)(*DAT_0071d20c + 0x30))(DAT_0071d20c);
      (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
      (**(code **)(*DAT_0071d174 + 400))(DAT_0071d174,0,DAT_0071d20c,0,iVar4);
      (**(code **)(*DAT_0071d174 + 0x1a0))(DAT_0071d174,DAT_0071d208);
      (**(code **)(*DAT_0071d174 + 0x148))(DAT_0071d174,6,0,0,4,0,2);
      (**(code **)(*DAT_0071d174 + 0x164))(DAT_0071d174,0);
    }
    if (DAT_006893e6 != '\0') {
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,8,2);
    }
  }
  if (DAT_006b3858 != '\0') {
    local_3c = 0x28001e0;
    local_40 = 6.44597e-43;
    FUN_00449780();
  }
  (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x13,5);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x14,6);
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xab,1);
  if (((DAT_00721ea4 != 0) && (DAT_0071d168 == 0)) &&
     (iVar4 = (*DAT_00721ebc)(DAT_00721ea4), iVar4 < 0)) {
    DAT_0071d168 = 1;
  }
  piVar6 = DAT_0071d174;
  (**(code **)(*DAT_0071d174 + 0x134))(DAT_0071d174,DAT_0069c680);
  cVar7 = (char)((uint)piVar6 >> 0x18);
  iVar4 = (**(code **)(*DAT_0071d174 + 0xa8))(DAT_0071d174);
  if ((-1 < iVar4) && (cVar7 != '\0')) {
    DAT_0071d16f = 0;
  }
  return;
}
#endif
