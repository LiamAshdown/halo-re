// text_parse_state_initialize  (Ghidra: FUN_00556b00, renamed)
// address 0x556b00, size 169 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: out/phase4/text_types_notes.md / types/text.h. Fills a caller-owned
//   text_parse_state: font, string, justification and style are copied straight from
//   the arguments, position starts at 0, color is the ColorARGB argument packed to
//   0xAARRGGBB (each channel truncated, not rounded, matching the four __ftol calls),
//   and font_definition is resolved by indexing the style's Font dependency
//   (Font.bold/italic/condense/underline at +0x48 + style * 0x10) off the base font,
//   falling back to the base font when that dependency is NONE (tag_id -1). Confirmed
//   by objdump (0x556b00..0x556ba8): the function reads only its own parameters plus
//   the tag_instances table and the 255.0 scale constant, never a text_* global -- every
//   caller happens to pass this module's own text_font / text_color /
//   text_justification_state / text_style_state globals through, but that is the
//   callers' choice, not something this function reads directly.
//   Named after the struct it fills (types/text.h already called it
//   text_parse_state_initialize); the first rewrite used text_parser_state_init.
// register convention: ECX = string, DX = justification, BX = style, ESI = out state
//   (unnamed hidden pointer arg, never saved/restored by this function), stack =
//   (font datum, ColorARGB*).
//   // blam-cc: ECX -> string, EDX -> justification, EBX -> style, ESI -> state, stack -> font, color

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "text.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances;             // 0x0087bc14
extern float text_color_scale;                  // 0x00672b60, == k_text_color_channel_scale

// blam-cc: ECX -> string, EDX -> justification, EBX -> style, ESI -> state, stack -> font, color
void text_parse_state_initialize(void *string, int16_t justification, int16_t style,
    text_parse_state *state, datum_index font, ColorARGB *color)
{
    datum_index resolved_font;

    state->font = font;
    state->string = (uint32_t)string;
    state->justification = justification;
    state->position = 0;
    state->style = style;

    state->color = ((uint32_t)(int32_t)(color->alpha * text_color_scale) << 24) |
                   ((uint32_t)(int32_t)(color->red   * text_color_scale) << 16) |
                   ((uint32_t)(int32_t)(color->green * text_color_scale) << 8) |
                   (uint32_t)(int32_t)(color->blue  * text_color_scale);

    resolved_font = font;
    if (style != (int16_t)-1) {
        Font *base_font = (Font *)tag_instances[font & 0xffff].data;
        // style indexes the four style dependencies (bold=0, italic=1, condense=2,
        // underline=3), which are laid out consecutively at the same stride as
        // TagDependency (0x10 bytes); see types/text.h for the confirmed offsets.
        TagDependency *style_dependency = &base_font->bold + style;
        resolved_font = *(datum_index *)&style_dependency->tag_id;
    }
    if (resolved_font == (datum_index)0xffffffff) {
        resolved_font = font;
    }
    state->font_definition = (uint32_t)tag_instances[resolved_font & 0xffff].data;
}

#if 0
Original Ghidra decompilation (0x556b00):

void FUN_00556b00(uint param_1)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint in_ECX;
  undefined2 in_DX;
  short unaff_BX;
  uint *unaff_ESI;

  *unaff_ESI = param_1;
  unaff_ESI[2] = in_ECX;
  *(undefined2 *)(unaff_ESI + 4) = in_DX;
  *(undefined2 *)(unaff_ESI + 3) = 0;
  *(short *)((int)unaff_ESI + 0xe) = unaff_BX;
  iVar2 = __ftol();
  uVar3 = __ftol();
  uVar4 = __ftol();
  uVar5 = __ftol();
  iVar1 = DAT_0087bc14;
  unaff_ESI[6] = ((iVar2 << 8 | uVar3) << 8 | uVar4) << 8 | uVar5;
  uVar3 = param_1;
  if (unaff_BX != -1) {
    uVar3 = *(uint *)(*(int *)((param_1 & 0xffff) * 0x20 + 0x14 + iVar1) + 0x48 + unaff_BX * 0x10);
  }
  if (uVar3 == 0xffffffff) {
    uVar3 = param_1;
  }
  unaff_ESI[1] = *(uint *)((uVar3 & 0xffff) * 0x20 + 0x14 + iVar1);
  return;
}

--- objdump -d -M intel (0x556b00..0x556bb0) ---

00556b00 <.text+0x155b00>:
  556b00:	8b 44 24 04             mov    eax,DWORD PTR [esp+0x4]
  556b04:	55                      push   ebp
  556b05:	57                      push   edi
  556b06:	8b 7c 24 10             mov    edi,DWORD PTR [esp+0x10]
  556b0a:	89 06                   mov    DWORD PTR [esi],eax
  556b0c:	89 4e 08                mov    DWORD PTR [esi+0x8],ecx
  556b0f:	66 89 56 10             mov    WORD PTR [esi+0x10],dx
  556b13:	66 c7 46 0c 00 00       mov    WORD PTR [esi+0xc],0x0
  556b19:	66 89 5e 0e             mov    WORD PTR [esi+0xe],bx
  556b1d:	d9 07                   fld    DWORD PTR [edi]
  556b1f:	d8 0d 60 2b 67 00       fmul   DWORD PTR ds:0x672b60
  556b25:	e8 8a 26 0e 00          call   0x6391b4
  556b2a:	d9 47 04                fld    DWORD PTR [edi+0x4]
  556b2d:	d8 0d 60 2b 67 00       fmul   DWORD PTR ds:0x672b60
  556b33:	8b e8                   mov    ebp,eax
  556b35:	c1 e5 08                shl    ebp,0x8
  556b38:	e8 77 26 0e 00          call   0x6391b4
  556b3d:	d9 47 08                fld    DWORD PTR [edi+0x8]
  556b40:	d8 0d 60 2b 67 00       fmul   DWORD PTR ds:0x672b60
  556b46:	0b e8                   or     ebp,eax
  556b48:	c1 e5 08                shl    ebp,0x8
  556b4b:	e8 64 26 0e 00          call   0x6391b4
  556b50:	d9 47 0c                fld    DWORD PTR [edi+0xc]
  556b53:	d8 0d 60 2b 67 00       fmul   DWORD PTR ds:0x672b60
  556b59:	0b e8                   or     ebp,eax
  556b5b:	c1 e5 08                shl    ebp,0x8
  556b5e:	e8 51 26 0e 00          call   0x6391b4
  556b63:	8b 4c 24 0c             mov    ecx,DWORD PTR [esp+0xc]
  556b67:	8b 15 14 bc 87 00       mov    edx,DWORD PTR ds:0x87bc14
  556b6d:	0b e8                   or     ebp,eax
  556b6f:	66 83 fb ff             cmp    bx,0xffff
  556b73:	89 6e 18                mov    DWORD PTR [esi+0x18],ebp
  556b76:	8b c1                   mov    eax,ecx
  556b78:	74 16                   je     0x556b90
  556b7a:	25 ff ff 00 00          and    eax,0xffff
  556b7f:	c1 e0 05                shl    eax,0x5
  556b82:	8b 44 10 14             mov    eax,DWORD PTR [eax+edx*1+0x14]
  556b86:	0f bf fb                movsx  edi,bx
  556b89:	c1 e7 04                shl    edi,0x4
  556b8c:	8b 44 38 48             mov    eax,DWORD PTR [eax+edi*1+0x48]
  556b90:	83 f8 ff                cmp    eax,0xffffffff
  556b93:	5f                      pop    edi
  556b94:	5d                      pop    ebp
  556b95:	75 02                   jne    0x556b99
  556b97:	8b c1                   mov    eax,ecx
  556b99:	25 ff ff 00 00          and    eax,0xffff
  556b9e:	c1 e0 05                shl    eax,0x5
  556ba1:	8b 4c 10 14             mov    ecx,DWORD PTR [eax+edx*1+0x14]
  556ba5:	89 4e 04                mov    DWORD PTR [esi+0x4],ecx
  556ba8:	c3                      ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
