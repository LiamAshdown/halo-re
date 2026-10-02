// hwreq_parser_parse_audiovendor_block  (Ghidra: FUN_0057aa40; renamed, still unclaimed by any
//   prior pass)
// address 0x57aa40, size 1032 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/shell_types_notes.md: "Parses an 'audiovendor[=id] { device = \"...\"
//   ... }'-style block from the requirements script, applying the enclosed directives only when
//   the id matches the detected audio-device vendor." Identical shape to
//   hwreq_parser_parse_vendor_block 0x57a680, with "audiovendor" in place of "vendor",
//   this->sound_device.vendor_id / device_id (+0x690 / +0x694 = hwreq_parser.sound_device
//   +0x638 plus shell_sound_device.vendor_id/device_id at +0x58/+0x5c) in place of
//   this->adapter.vendor_id / device_id, and this->sound_vendor_name / sound_device_name
//   (+0x94 / +0x78) in place of this->graphics_vendor_name / graphics_device_name. The nested
//   scan additionally stops on a bare "vendor" line (not just another "audiovendor" line).
// register convention: this in EAX at entry (objdump: "mov esi,eax" first instruction, then ESI
//   used throughout; confirmed identical to the vendor-block twin). Same callee register
//   conventions as hwreq_parser_parse_vendor_block: hwreq_token_parse_number / hwreq_token_
//   parse_quoted_string / hwreq_token_skip_line take this in EAX; hwreq_token_skip_whitespace
//   takes this in EDX; hwreq_parser_report_error takes this in ESI (live-in); msvc_string_
//   assign_n (FUN_0057bc90) is ECX = destination, source/length on the stack (confirmed here:
//   "lea ecx,[esi+0x94]" before the call); hwreq_string_assign_cstr (FUN_0057b590) takes the
//   source in EDX and the destination on the stack (confirmed here: "lea edx,[esi+0x78]; push
//   edx; mov edx,eax").
// blam-cc: this in EAX (only parameter).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hwreq_parser_report_error(hwreq_parser *self, const char *message); // 0x578a20, blam-cc: this in ESI (live-in), message on the stack; below this module's rewrite range
extern int32_t hwreq_token_parse_number(hwreq_parser *self); // 0x578b20, blam-cc: this in EAX; below this module's rewrite range; -1 on error
extern char *hwreq_token_parse_quoted_string(hwreq_parser *self); // 0x578c60, blam-cc: this in EAX; below this module's rewrite range; NULL on error
extern void hwreq_token_skip_whitespace(hwreq_parser *self); // 0x578a00, blam-cc: this in EDX; below this module's rewrite range
extern void hwreq_token_skip_line(hwreq_parser *self); // 0x5789d0, blam-cc: this in EAX; below this module's rewrite range
extern void msvc_string_assign_n(msvc_std_string *dest, const char *source, uint32_t length); // 0x57bc90, blam-cc: dest in ECX, source/length on the stack; library code, not in the function list
extern msvc_std_string *hwreq_string_assign_cstr(msvc_std_string *dest, const char *source); // 0x57b590, blam-cc: source in EDX, dest on the stack; library code, not in the function list
extern uint8_t hwreq_parser_parse_block(hwreq_parser *self, hwreq_property_set *target); // 0x57af10

// Scans forward from the parser's cursor for an "audiovendor[=id] = \"name\" { ... }" directive
// whose id matches the detected sound device vendor (or the literal keyword "unknown", which
// always matches), records the vendor name, and then parses the directive's nested lines: a
// "device[=id] = \"name\"" or "unknown = \"name\"" line whose id matches the detected device
// records the device name and parses its own trailing { ... } block into this->flags; any other
// nested line is parsed directly as a flags block. A non-matching "audiovendor" line at the top
// level is skipped for its single line only; reaching "applytoall", or the end of the file,
// without ever entering a matching audiovendor block returns true with the cursor left there.
uint8_t hwreq_parser_parse_audiovendor_block(hwreq_parser *self)
{
    char *cursor;
    char *line;
    char c;
    int32_t vendor_id;
    int32_t device_id;
    char *name;
    uint32_t length;
    uint8_t ok;

    for (;;) {
        if (_strnicmp((char *)self->cursor, "audiovendor", 11) == 0) {
            c = ((char *)self->cursor)[11];
            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                cursor = (char *)self->cursor + 11;
                while (*cursor == ' ' || *cursor == '\t') {
                    cursor++;
                }
                self->cursor = (uint32_t)cursor;

                if (*cursor == '=') {
                    cursor++;
                    self->cursor = (uint32_t)cursor;

                    if (_strnicmp(cursor, "unknown", 7) == 0 &&
                        (c = cursor[7], c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t')) {
                        vendor_id = (int32_t)self->sound_device.vendor_id;
                    } else {
                        vendor_id = hwreq_token_parse_number(self);
                        if (vendor_id != (int32_t)self->sound_device.vendor_id) {
                            goto skip_line_and_rescan;
                        }
                    }
                    if (vendor_id == -1) {
                        return 1;
                    }

                    name = hwreq_token_parse_quoted_string(self);
                    if (name == 0) {
                        return 0;
                    }
                    length = 0;
                    while (name[length] != 0) {
                        length++;
                    }
                    msvc_string_assign_n(&self->sound_vendor_name, name, length);

                    for (;;) {
                        // consume the rest of the current line
                        do {
                            line = (char *)self->cursor;
                            self->cursor = (uint32_t)(line + 1);
                            if (*line == '\r') break;
                        } while ((char *)self->cursor < (char *)self->end);
                        if ((char *)self->cursor < (char *)self->end && *(char *)self->cursor == '\n') {
                            self->cursor = (uint32_t)(line + 2);
                        }
                        self->line_start = self->cursor;
                        self->line_number = self->line_number + 1;

                        if (!(_strnicmp((char *)self->cursor, "audiovendor", 11) == 0 &&
                              (c = ((char *)self->cursor)[11],
                               c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t'))) {
                            cursor = (char *)self->cursor;
                            while (*cursor == ' ' || *cursor == '\t') {
                                cursor++;
                            }
                            self->cursor = (uint32_t)cursor;
                            c = *cursor;
                            if (c < '0' || c > '9') {
                                if (c != '\r' && *(uint16_t *)cursor != 0x2f2f /* "//" */) {
                                    ok = hwreq_parser_parse_block(self, (hwreq_property_set *)self->flags);
                                    if (ok == 0) {
                                        return 0;
                                    }
                                }
                                goto after_nested_line;
                            }
                            // a digit: fall through into the device-id scan below
                            break;
                        }
                    after_nested_line:
                        if (!((char *)self->cursor < (char *)self->end)) {
                            break;
                        }
                    }

                    for (;;) {
                        if (_strnicmp((char *)self->cursor, "audiovendor", 11) == 0) {
                            c = ((char *)self->cursor)[11];
                            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                                return 1;
                            }
                        }
                        if (_strnicmp((char *)self->cursor, "vendor", 6) == 0) {
                            c = ((char *)self->cursor)[6];
                            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                                return 1;
                            }
                        }
                        if (_strnicmp((char *)self->cursor, "unknown", 7) == 0) {
                            c = ((char *)self->cursor)[7];
                            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                                device_id = (int32_t)self->sound_device.device_id;
                                self->cursor = self->cursor + 7;
                                hwreq_token_skip_whitespace(self);
                                goto check_device_id;
                            }
                        }
                        cursor = (char *)self->cursor;
                        while (*cursor == ' ' || *cursor == '\t') {
                            cursor++;
                        }
                        self->cursor = (uint32_t)cursor;
                        if (*cursor >= '0' && *cursor <= '9') {
                            device_id = hwreq_token_parse_number(self);
                            if (device_id == (int32_t)self->sound_device.device_id) {
                                goto check_device_id;
                            }
                        }

                        do {
                            line = (char *)self->cursor;
                            self->cursor = (uint32_t)(line + 1);
                            if (*line == '\r') break;
                        } while ((char *)self->cursor < (char *)self->end);
                        if ((char *)self->cursor < (char *)self->end && *(char *)self->cursor == '\n') {
                            self->cursor = (uint32_t)(line + 2);
                        }
                        self->line_start = self->cursor;
                        self->line_number = self->line_number + 1;
                        if ((char *)self->end <= (char *)self->cursor) {
                            return 1;
                        }
                        continue;

                    check_device_id:
                        if (device_id == -1) {
                            return 1;
                        }
                        if (*(char *)self->cursor == '=') {
                            self->cursor = self->cursor + 1;
                            name = hwreq_token_parse_quoted_string(self);
                            if (name != 0) {
                                hwreq_string_assign_cstr(&self->sound_device_name, name);
                                hwreq_token_skip_line(self);
                                ok = hwreq_parser_parse_block(self, (hwreq_property_set *)self->flags);
                                return ok != 0;
                            }
                        } else {
                            hwreq_parser_report_error(self, "xxx = Device Name expected");
                        }
                        return 0;
                    }
                }
            }
        } else if (_strnicmp((char *)self->cursor, "applytoall", 10) == 0) {
            c = ((char *)self->cursor)[10];
            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                return 1;
            }
        }

    skip_line_and_rescan:
        do {
            line = (char *)self->cursor;
            self->cursor = (uint32_t)(line + 1);
            if (*line == '\r') break;
        } while ((char *)self->cursor < (char *)self->end);
        if ((char *)self->cursor < (char *)self->end && *(char *)self->cursor == '\n') {
            self->cursor = (uint32_t)(line + 2);
        }
        self->line_start = self->cursor;
        self->line_number = self->line_number + 1;
        if ((char *)self->end <= (char *)self->cursor) {
            return 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x57aa40):

bool FUN_0057aa40(void)

{
  char cVar1;
  int in_EAX;
  int iVar2;
  char *pcVar3;
  char *pcVar4;

LAB_0057aa50:
  iVar2 = __strnicmp(*(char **)(in_EAX + 8),"audiovendor",0xb);
  if (iVar2 == 0) {
    pcVar3 = (char *)(*(int *)(in_EAX + 8) + 0xb);
    cVar1 = *pcVar3;
    if ((((((cVar1 != '>') && (cVar1 != '<')) && (cVar1 != '!')) &&
         ((cVar1 != '=' && (cVar1 != ' ')))) && (cVar1 != '\r')) && (cVar1 != '\t'))
    goto LAB_0057ab1a;
    while( true ) {
      *(char **)(in_EAX + 8) = pcVar3;
      if ((*pcVar3 != ' ') && (*pcVar3 != '\t')) break;
      pcVar3 = pcVar3 + 1;
    }
    if (**(char **)(in_EAX + 8) == '=') {
      pcVar3 = *(char **)(in_EAX + 8) + 1;
      *(char **)(in_EAX + 8) = pcVar3;
      iVar2 = __strnicmp(pcVar3,"unknown",7);
      if ((iVar2 == 0) &&
         ((((cVar1 = *(char *)(*(int *)(in_EAX + 8) + 7), cVar1 == '>' || (cVar1 == '<')) ||
           (cVar1 == '!')) ||
          (((cVar1 == '=' || (cVar1 == ' ')) || ((cVar1 == '\r' || (cVar1 == '\t')))))))) {
        iVar2 = *(int *)(in_EAX + 0x690);
      }
      else {
        iVar2 = hwreq_token_parse_number();
        if (iVar2 != *(int *)(in_EAX + 0x690)) goto LAB_0057ab53;
      }
      if (iVar2 == -1) {
        return true;
      }
      pcVar3 = (char *)hwreq_token_parse_quoted_string();
      if (pcVar3 == (char *)0x0) {
        return false;
      }
      pcVar4 = pcVar3;
      do {
        cVar1 = *pcVar4;
        pcVar4 = pcVar4 + 1;
      } while (cVar1 != '\0');
      FUN_0057bc90(pcVar3,(int)pcVar4 - (int)(pcVar3 + 1));
LAB_0057abd0:
      do {
        pcVar3 = *(char **)(in_EAX + 8);
        pcVar4 = pcVar3 + 1;
        *(char **)(in_EAX + 8) = pcVar4;
        if (*pcVar3 != '\r') {
          if (pcVar4 < *(char **)(in_EAX + 0xc)) goto LAB_0057abd0;
        }
        if ((pcVar4 < *(char **)(in_EAX + 0xc)) && (*pcVar4 == '\n')) {
          *(char **)(in_EAX + 8) = pcVar3 + 2;
        }
        *(char **)(in_EAX + 0x10) = *(char **)(in_EAX + 8);
        *(int *)(in_EAX + 0x14) = *(int *)(in_EAX + 0x14) + 1;
        iVar2 = __strnicmp(*(char **)(in_EAX + 8),"audiovendor",0xb);
        if ((iVar2 != 0) ||
           ((((cVar1 = *(char *)(*(int *)(in_EAX + 8) + 0xb), cVar1 != '>' && (cVar1 != '<')) &&
             (cVar1 != '!')) &&
            (((cVar1 != '=' && (cVar1 != ' ')) && ((cVar1 != '\r' && (cVar1 != '\t')))))))) {
          while( true ) {
            cVar1 = **(char **)(in_EAX + 8);
            if ((cVar1 != ' ') && (cVar1 != '\t')) break;
            *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
          }
          cVar1 = (char)**(short **)(in_EAX + 8);
          if ((cVar1 < '0') || ('9' < cVar1)) {
            if (((cVar1 != '\r') && (**(short **)(in_EAX + 8) != 0x2f2f)) &&
               (cVar1 = hwreq_parser_parse_block(in_EAX,*(undefined4 *)(in_EAX + 0x18)),
               cVar1 == '\0')) {
              return false;
            }
            goto LAB_0057ac71;
          }
          break;
        }
LAB_0057ac71:
      } while (*(uint *)(in_EAX + 8) < *(uint *)(in_EAX + 0xc));
      do {
        iVar2 = __strnicmp(*(char **)(in_EAX + 8),"audiovendor",0xb);
        if (iVar2 == 0) {
          cVar1 = *(char *)(*(int *)(in_EAX + 8) + 0xb);
          if (cVar1 == '>') {
            return true;
          }
          if (cVar1 == '<') {
            return true;
          }
          if (cVar1 == '!') {
            return true;
          }
          if (cVar1 == '=') {
            return true;
          }
          if (cVar1 == ' ') {
            return true;
          }
          if (cVar1 == '\r') {
            return true;
          }
          if (cVar1 == '\t') {
            return true;
          }
        }
        iVar2 = __strnicmp(*(char **)(in_EAX + 8),"vendor",6);
        if (iVar2 == 0) {
          cVar1 = *(char *)(*(int *)(in_EAX + 8) + 6);
          if (cVar1 == '>') {
            return true;
          }
          if (cVar1 == '<') {
            return true;
          }
          if (cVar1 == '!') {
            return true;
          }
          if (cVar1 == '=') {
            return true;
          }
          if (cVar1 == ' ') {
            return true;
          }
          if (cVar1 == '\r') {
            return true;
          }
          if (cVar1 == '\t') {
            return true;
          }
        }
        iVar2 = __strnicmp(*(char **)(in_EAX + 8),"unknown",7);
        if ((iVar2 == 0) &&
           (((((cVar1 = *(char *)(*(int *)(in_EAX + 8) + 7), cVar1 == '>' || (cVar1 == '<')) ||
              ((cVar1 == '!' || ((cVar1 == '=' || (cVar1 == ' ')))))) || (cVar1 == '\r')) ||
            (cVar1 == '\t')))) {
          iVar2 = *(int *)(in_EAX + 0x694);
          *(int *)(in_EAX + 8) = *(int *)(in_EAX + 8) + 7;
          hwreq_token_skip_whitespace();
LAB_0057adfa:
          if (iVar2 == -1) {
            return true;
          }
          if (**(char **)(in_EAX + 8) == '=') {
            *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
            iVar2 = hwreq_token_parse_quoted_string();
            if (iVar2 != 0) {
              FUN_0057b590(in_EAX + 0x78);
              hwreq_token_skip_line();
              cVar1 = hwreq_parser_parse_block(in_EAX,*(undefined4 *)(in_EAX + 0x18));
              return cVar1 != '\0';
            }
          }
          else {
            hwreq_parser_report_error("xxx = Device Name expected");
          }
          return false;
        }
        while( true ) {
          cVar1 = **(char **)(in_EAX + 8);
          if ((cVar1 != ' ') && (cVar1 != '\t')) break;
          *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
        }
        if (('/' < **(char **)(in_EAX + 8)) &&
           ((**(char **)(in_EAX + 8) < ':' &&
            (iVar2 = hwreq_token_parse_number(), iVar2 == *(int *)(in_EAX + 0x694)))))
        goto LAB_0057adfa;
        do {
          pcVar3 = *(char **)(in_EAX + 8);
          pcVar4 = pcVar3 + 1;
          *(char **)(in_EAX + 8) = pcVar4;
          if (*pcVar3 == '\r') break;
        } while (pcVar4 < *(char **)(in_EAX + 0xc));
        if ((pcVar4 < *(char **)(in_EAX + 0xc)) && (*pcVar4 == '\n')) {
          *(char **)(in_EAX + 8) = pcVar3 + 2;
        }
        *(char **)(in_EAX + 0x10) = *(char **)(in_EAX + 8);
        *(int *)(in_EAX + 0x14) = *(int *)(in_EAX + 0x14) + 1;
        if (*(char **)(in_EAX + 0xc) <= *(char **)(in_EAX + 8)) {
          return true;
        }
      } while( true );
    }
  }
  else {
LAB_0057ab1a:
    iVar2 = __strnicmp(*(char **)(in_EAX + 8),"applytoall",10);
    if (iVar2 == 0) {
      cVar1 = *(char *)(*(int *)(in_EAX + 8) + 10);
      if (cVar1 == '>') {
        return true;
      }
      if (cVar1 == '<') {
        return true;
      }
      if (cVar1 == '!') {
        return true;
      }
      if (cVar1 == '=') {
        return true;
      }
      if (cVar1 == ' ') {
        return true;
      }
      if (cVar1 == '\r') {
        return true;
      }
      if (cVar1 == '\t') {
        return true;
      }
    }
  }
LAB_0057ab53:
  do {
    pcVar3 = *(char **)(in_EAX + 8);
    pcVar4 = pcVar3 + 1;
    *(char **)(in_EAX + 8) = pcVar4;
    if (*pcVar3 == '\r') break;
  } while (pcVar4 < *(char **)(in_EAX + 0xc));
  if ((pcVar4 < *(char **)(in_EAX + 0xc)) && (*pcVar4 == '\n')) {
    *(char **)(in_EAX + 8) = pcVar3 + 2;
  }
  *(char **)(in_EAX + 0x10) = *(char **)(in_EAX + 8);
  *(int *)(in_EAX + 0x14) = *(int *)(in_EAX + 0x14) + 1;
  if (*(char **)(in_EAX + 0xc) <= *(char **)(in_EAX + 8)) {
    return true;
  }
  goto LAB_0057aa50;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
