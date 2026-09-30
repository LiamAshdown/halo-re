// text_parse_next_token_narrow  (Ghidra: FUN_00556bb0, renamed)
// address 0x556bb0, size 787 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/text_types_notes.md / types/text.h describe this as the narrow
//   (8-bit, DBCS-aware) tokenizer paired with text_draw_character_range_narrow's own
//   naming for its callee prototype (src/text/text_draw_character_range_narrow.c, a
//   sibling file in this module). Reads one character (1 or 2 bytes, DBCS-aware) from
//   state->string at state->position, advances position, and classifies it:
//     - a '|x' markup escape recognized by the switch below sets style/justification
//       and returns text_token_style (consumed internally: the tokenizer loops back
//       and reads the next character instead of returning) or text_token_justification
//       (returned to the caller);
//     - NUL / TAB / CR return text_token_end / text_token_tab / text_token_newline;
//     - any other character is classified as a break character (token 2) or an
//       ordinary character (token 6) using the three character sets cached from the
//       active localization string list (indices 4, 5, 6): a single-byte character
//       must be in the single-byte break-character set (index 4); a double-byte
//       character must NOT be in the double-byte no-break set (index 5); and in both
//       cases the character that FOLLOWS it must not be in the no-break-character set
//       (index 6) -- confirmed from objdump: the third FUN_00557870 search is passed
//       the character at the (already advanced) position, i.e. the next character, not
//       the one just classified. (out/phase4/text_types_notes.md's "not in string 6"
//       wording does not call this out; this rewrite follows the disassembly.)
// register convention: EDI = state, no other arguments (the string/position/font it
//   reads are all fields of *state).
//   // blam-cc: EDI -> state
// text_find_character (0x557870): EBX = target character, stack = string, confirmed
//   in both its prologue and the three call sites here (0x556e12 mov ebx,eax / push).

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "text.h"
#include "fn_text.h"
#include <ctype.h>

extern tag_instance *tag_instances;                 // 0x0087bc14
extern datum_index text_localization_strings;       // 0x006e4728

extern char missing_string[17];                    // 0x00671fd0, "<missing string>"

// blam-cc: EAX -> string; result in AL only
extern uint8_t text_char_is_double_byte(uint8_t *string); // 0x557750
// blam-cc: EBX -> target_character, stack -> string; result in AL only


// blam-cc: EDI -> state
int16_t text_parse_next_token_narrow(text_parse_state *state)
{
    uint8_t *position;
    uint16_t ch;
    int16_t token;

    for (;;) {
        position = (uint8_t *)state->string + state->position;
        if (text_char_is_double_byte(position)) {
            ch = ((uint16_t)position[0] << 8) | position[1];
            state->position = (int16_t)(state->position + 2);
        } else {
            ch = position[0];
            state->position = (int16_t)(state->position + 1);
        }

        token = -1;
        if ((ch & 0xff00) == 0x7c00) { // '|' lead byte
            switch (tolower(ch & 0xff)) {
            case 'b': state->style = _text_style_bold;      token = _text_token_style; break;
            case 'c': state->justification = _text_justification_center; token = _text_token_justification; break;
            case 'i': state->style = _text_style_italic;    token = _text_token_style; break;
            case 'k': state->style = _text_style_condense;  token = _text_token_style; break;
            case 'l': state->justification = _text_justification_left;  token = _text_token_justification; break;
            case 'n': token = _text_token_newline; break;
            case 'p': state->style = _text_style_plain;     token = _text_token_style; break;
            case 'r': state->justification = _text_justification_right; token = _text_token_justification; break;
            case 't': token = _text_token_tab; break;
            case 'u': state->style = _text_style_underline; token = _text_token_style; break;
            }
        }

        if (token == -1) {
            break; // not a recognized markup code: fall into the plain-character path below
        }

        if (token == _text_token_style) {
            // Re-resolve font_definition for the new style, the same way
            // text_parse_state_initialize (0x556b00) does.
            datum_index resolved_font = state->font;
            if (state->style != (int16_t)-1) {
                Font *base_font = (Font *)tag_instances[state->font & 0xffff].data;
                TagDependency *style_dependency = &base_font->bold + state->style;
                resolved_font = *(datum_index *)&style_dependency->tag_id;
                if (resolved_font == (datum_index)0xffffffff) {
                    resolved_font = state->font;
                }
            }
            state->font_definition = (uint32_t)tag_instances[resolved_font & 0xffff].data;
        }

        if (token != _text_token_style && token != _text_token_unused_5) {
            state->token = token;
            state->character = ch;
            return token;
        }
        // style codes (and the never-produced token 5) are consumed here: loop back
        // and read the next character instead of returning.
    }

    if (ch == 0) {
        state->token = _text_token_end;
        state->character = 0;
        return _text_token_end;
    }
    if (ch == '\t') {
        state->token = _text_token_tab;
        state->character = ch;
        return _text_token_tab;
    }
    if (ch == '\r') {
        state->token = _text_token_newline;
        state->character = ch;
        return _text_token_newline;
    }

    {
        int is_double_byte_char = (ch & 0xff00) != 0;
        uint8_t *next_position = (uint8_t *)state->string + state->position;
        uint16_t lookahead_char;
        StringList *localization;
        char *single_byte_break_characters = missing_string;
        char *double_byte_no_break_characters = missing_string;
        char *no_break_characters = missing_string;
        int is_break_character;

        if (text_char_is_double_byte(next_position)) {
            lookahead_char = ((uint16_t)next_position[0] << 8) | next_position[1];
        } else {
            lookahead_char = next_position[0];
        }

        if (text_localization_strings != (datum_index)k_datum_index_none) {
            localization = (StringList *)tag_instances[text_localization_strings & 0xffff].data;

            if (localization->strings.count > _text_localization_single_byte_break_characters) {
                StringListString *entry = (StringListString *)localization->strings.pointer +
                    _text_localization_single_byte_break_characters;
                if ((int32_t)entry->string.size > 0) {
                    single_byte_break_characters = (char *)entry->string.pointer;
                    single_byte_break_characters[entry->string.size - 1] = '\0';
                }
            }
            if (localization->strings.count > _text_localization_double_byte_no_break_characters) {
                StringListString *entry = (StringListString *)localization->strings.pointer +
                    _text_localization_double_byte_no_break_characters;
                if ((int32_t)entry->string.size > 0) {
                    double_byte_no_break_characters = (char *)entry->string.pointer;
                    double_byte_no_break_characters[entry->string.size - 1] = '\0';
                }
            }
            if (localization->strings.count > _text_localization_no_break_characters) {
                StringListString *entry = (StringListString *)localization->strings.pointer +
                    _text_localization_no_break_characters;
                if ((int32_t)entry->string.size > 0) {
                    no_break_characters = (char *)entry->string.pointer;
                    no_break_characters[entry->string.size - 1] = '\0';
                }
            }
        }

        if (!is_double_byte_char) {
            is_break_character = text_find_character((int16_t)ch,
                (uint8_t *)single_byte_break_characters) != 0;
        } else {
            is_break_character = text_find_character((int16_t)ch,
                (uint8_t *)double_byte_no_break_characters) == 0;
        }
        if (is_break_character) {
            is_break_character = text_find_character((int16_t)lookahead_char,
                (uint8_t *)no_break_characters) == 0;
        }

        token = is_break_character ? _text_token_break_character : _text_token_character;
        state->token = token;
        state->character = ch;
        return token;
    }
}

#if 0
Original Ghidra decompilation (0x556bb0):

short FUN_00556bb0(void)

{
  byte bVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  char cVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  undefined2 uVar9;
  char *pcVar10;
  char *pcVar11;
  short sVar12;
  byte *pbVar13;
  uint *unaff_EDI;
  uint local_c;
  char *local_8;

  while( true ) {
    pbVar13 = (byte *)((int)(short)unaff_EDI[3] + unaff_EDI[2]);
    cVar5 = text_char_is_double_byte();
    if (cVar5 == '\0') {
      bVar1 = *pbVar13;
      *(short *)(unaff_EDI + 3) = (short)unaff_EDI[3] + 1;
      local_c = (uint)bVar1;
    }
    else {
      local_c = (uint)CONCAT11(*pbVar13,pbVar13[1]);
      *(short *)(unaff_EDI + 3) = (short)unaff_EDI[3] + 2;
    }
    sVar12 = -1;
    if ((local_c & 0xff00) == 0x7c00) {
      iVar6 = _tolower(local_c & 0xff);
      switch(iVar6) {
      case 0x62:
        *(undefined2 *)((int)unaff_EDI + 0xe) = 0;
        sVar12 = 7;
        break;
      case 99:
        *(undefined2 *)(unaff_EDI + 4) = 2;
        sVar12 = 4;
        break;
      case 0x69:
        *(undefined2 *)((int)unaff_EDI + 0xe) = 1;
        sVar12 = 7;
        break;
      case 0x6b:
        *(undefined2 *)((int)unaff_EDI + 0xe) = 2;
        sVar12 = 7;
        break;
      case 0x6c:
        *(undefined2 *)(unaff_EDI + 4) = 0;
        sVar12 = 4;
        break;
      case 0x6e:
        sVar12 = 1;
        break;
      case 0x70:
        *(undefined2 *)((int)unaff_EDI + 0xe) = 0xffff;
        sVar12 = 7;
        break;
      case 0x72:
        *(undefined2 *)(unaff_EDI + 4) = 1;
        sVar12 = 4;
        break;
      case 0x74:
        sVar12 = 3;
        break;
      case 0x75:
        *(undefined2 *)((int)unaff_EDI + 0xe) = 3;
        sVar12 = 7;
      }
    }
    uVar9 = (undefined2)local_c;
    if (sVar12 == -1) break;
    if (sVar12 == 7) {
      uVar7 = *unaff_EDI;
      uVar8 = uVar7;
      if (*(short *)((int)unaff_EDI + 0xe) != -1) {
        uVar8 = *(uint *)(*(short *)((int)unaff_EDI + 0xe) * 0x10 + 0x48 +
                         *(int *)(DAT_0087bc14 + 0x14 + (uVar7 & 0xffff) * 0x20));
      }
      if (uVar8 != 0xffffffff) {
        uVar7 = uVar8;
      }
      unaff_EDI[1] = *(uint *)((uVar7 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    }
    if ((sVar12 != 7) && (sVar12 != 5)) {
      *(short *)(unaff_EDI + 5) = sVar12;
      *(undefined2 *)((int)unaff_EDI + 0x12) = uVar9;
      return sVar12;
    }
  }
  if (local_c == 0) {
    *(undefined2 *)(unaff_EDI + 5) = 0;
    *(undefined2 *)((int)unaff_EDI + 0x12) = 0;
    return 0;
  }
  if (local_c == 9) {
    *(undefined2 *)(unaff_EDI + 5) = 3;
    *(undefined2 *)((int)unaff_EDI + 0x12) = 9;
    return 3;
  }
  if (local_c == 0xd) {
    *(undefined2 *)(unaff_EDI + 5) = 1;
    *(undefined2 *)((int)unaff_EDI + 0x12) = 0xd;
    return 1;
  }
  text_char_is_double_byte();
  iVar6 = DAT_0087bc14;
  pcVar11 = "<missing string>";
  pcVar10 = "<missing string>";
  if ((DAT_006e4728 != 0xffffffff) &&
     (piVar2 = *(int **)((DAT_006e4728 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 4 < *piVar2)) {
    iVar3 = piVar2[1];
    iVar4 = *(int *)(iVar3 + 0x50);
    if (0 < iVar4) {
      pcVar10 = *(char **)(iVar3 + 0x5c);
      pcVar10[iVar4 + -1] = '\0';
    }
  }
  local_8 = "<missing string>";
  if (DAT_006e4728 != 0xffffffff) {
    piVar2 = *(int **)((DAT_006e4728 & 0xffff) * 0x20 + 0x14 + iVar6);
    if (5 < *piVar2) {
      iVar3 = piVar2[1];
      iVar4 = *(int *)(iVar3 + 100);
      if (0 < iVar4) {
        local_8 = *(char **)(iVar3 + 0x70);
        local_8[iVar4 + -1] = '\0';
      }
    }
    if ((DAT_006e4728 != 0xffffffff) &&
       (piVar2 = *(int **)((DAT_006e4728 & 0xffff) * 0x20 + 0x14 + iVar6), 6 < *piVar2)) {
      iVar6 = piVar2[1];
      iVar3 = *(int *)(iVar6 + 0x78);
      if (0 < iVar3) {
        pcVar11 = *(char **)(iVar6 + 0x84);
        pcVar11[iVar3 + -1] = '\0';
      }
    }
  }
  if (((((local_c & 0xff00) != 0) || (cVar5 = FUN_00557870(pcVar10), cVar5 != '\0')) &&
      (((short)(local_c & 0xff00) == 0 || (cVar5 = FUN_00557870(local_8), cVar5 == '\0')))) &&
     (cVar5 = FUN_00557870(pcVar11), cVar5 == '\0')) {
    *(undefined2 *)(unaff_EDI + 5) = 2;
    *(undefined2 *)((int)unaff_EDI + 0x12) = uVar9;
    return 2;
  }
  *(undefined2 *)(unaff_EDI + 5) = 6;
  *(undefined2 *)((int)unaff_EDI + 0x12) = uVar9;
  return 6;
}

--- objdump -d -M intel (0x556bb0..0x556e98), key excerpts ---

00556bb0 <.text+0x155bb0>:
  556bb0: sub esp,0xc / push ebx,ebp,esi
  556bb6: movsx esi,[edi+0xc]; add esi,[edi+8]          ; esi = state->string + state->position
  556bbf: mov eax,esi; call 0x557750                      ; text_char_is_double_byte(esi)
  556bc6: mov ebp,0x2
  556bcb: je 0x556bde
  556bcd: xor ebx,ebx; mov bh,[esi]; mov bl,[esi+1]
  556bd4: add [edi+0xc],bp                                 ; position += 2
  556bd8: mov [esp+0xc],ebx                                 ; local_c = (hi<<8)|lo
  556bdc: jmp 0x556bef
  556bde: movzx ax,[esi]
  556be2: inc word[edi+0xc]                                  ; position += 1
  556be6: mov [esp+0xc],ax
  556beb: mov ebx,[esp+0xc]
  556bef: mov ecx,ebx; and ecx,0xff00; or esi,0xffffffff
  556bfa: cmp ecx,0x7c00; jne 0x556c9d                        ; not '|xx' -> sVar12 stays -1
  556c06: mov edx,ebx; and edx,0xff; push edx; call 0x624687  ; tolower(low byte)
  ; switch on tolower result (0x556ec4 jump table, 0x556ef0 case-index table) sets
  ; esi (token) and one of [edi+0xe] (style) or [edi+0x10] (justification)
  556c9d: movsx eax,si; cmp eax,-1; je 0x556d15               ; token==-1 -> fallback dispatch
  556ca5: cmp eax,7; jne 0x556cef
  556caa: ; style-resolve block (mirrors text_parse_state_initialize's font lookup), writes [edi+4]
  556cef: cmp si,7; je 0x556bb6                                ; style: loop back for next char
  556cf9: cmp si,5; je 0x556bb6                                ; token 5: also loops (never produced)
  556d03: mov [edi+0x14],si; mov [edi+0x12],bx; ret            ; token in {1,3,4}: return it

  556d15: movzx eax,bx; test eax,eax; je 0x556eaf              ; ch==0 -> token 0
  556d20: cmp eax,9; je 0x556e98                                 ; ch==9 -> token 3
  556d29: cmp eax,0xd; je 0x556e81                                ; ch==0xd -> token 1
  556d32: movsx esi,[edi+0xc]; add esi,[edi+8]                     ; esi = string + NEW position
  556d39: mov eax,esi; call 0x557750                                ; peek: is the NEXT char double-byte?
  556d44/556d4d: read 1 or 2 bytes at esi into eax -> [esp+0x14]    ; lookahead_char
  556d5e..556dfe: resolve string-list entries 4, 5, 6 (pcVar10, local_8, pcVar11)
  556e03: eax=[esp+0xc] (=local_c=ch); esi=eax&0xff00
  556e0f: jne 0x556e22                                              ; double-byte -> skip call1
  556e11: push ebx(pcVar10); ebx=eax(ch); call 0x557870               ; call1: set4 vs ch
  556e1c: test al,al; je 0x556e66                                      ; not found -> token 6
  556e22: test si,si; je 0x556e3a                                       ; single-byte -> skip call2
  556e27: push [esp+0x10](local_8); ebx=eax(ch); call 0x557870           ; call2: set5 vs ch
  556e36: test al,al; jne 0x556e66                                        ; found -> token 6
  556e3a: ebx=[esp+0x14](lookahead_char); push ebp(pcVar11); call 0x557870 ; call3: set6 vs lookahead
  556e47: test al,al; jne 0x556e66                                          ; found -> token 6
  556e4f: token = 2 (break character); store and return
  556e66: token = 6 (ordinary character); store and return
  556e81: token = 1 (newline, ch==0xd); store and return
  556e98: token = 3 (tab, ch==9); store and return
  556eaf: token = 0 (end, ch==0); store and return
#endif
