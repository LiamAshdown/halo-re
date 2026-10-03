// hwreq_parser_parse_vendor_block  (Ghidra: FUN_0057a680; renamed, still unclaimed by any prior
//   pass)
// address 0x57a680, size 951 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/shell_types_notes.md: "Parses a 'vendor[=id] { device = \"...\" ... }'
//   -style block from the requirements script, applying the enclosed directives only when the
//   id matches the detected graphics vendor." The "unknown" keyword and a numeric id both compare
//   against this->adapter.vendor_id (+0x4e4 = hwreq_parser.adapter.vendor_id, d3d_adapter_
//   identifier9 +0x428 at parser +0xbc); a nested "device=<id> = \"name\"" or "unknown = \"name\""
//   line similarly compares this->adapter.device_id (+0x4e8) and assigns
//   this->graphics_device_name (+0x40); the vendor line itself assigns
//   this->graphics_vendor_name (+0x5c); other nested lines are parsed as an ordinary block into
//   this->flags via hwreq_parser_parse_block.
// register convention: this in EAX at entry (objdump: "mov esi,eax" first instruction, then ESI
//   used throughout). hwreq_token_parse_number 0x578b20 and hwreq_token_parse_quoted_string
//   0x578c60 take this in EAX; hwreq_token_skip_whitespace 0x578a00 takes this in EDX;
//   hwreq_token_skip_line 0x5789d0 takes this in EAX; hwreq_parser_report_error 0x578a20 takes
//   this in ESI (live-in) with the message on the stack. The two library string helpers:
//   FUN_0057bc90 (declared here as msvc_string_assign_n) is a thiscall, ECX = destination
//   string, source pointer and length on the stack; FUN_0057b590 (declared here as
//   hwreq_string_assign_cstr) takes the source C string in EDX and the destination string on the
//   stack (confirmed by disassembling 0x57b590 itself: it strlens EDX and tailcalls
//   msvc_string_assign_n with the dest read back off its own stack argument).
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

// Scans forward from the parser's cursor for a "vendor[=id] = \"name\" { ... }" directive whose
// id matches the detected graphics vendor (or the literal keyword "unknown", which always
// matches), records the vendor name, and then parses the directive's nested lines: a
// "device[=id] = \"name\"" or "unknown = \"name\"" line whose id matches the detected device
// records the device name and parses its own trailing { ... } block into this->flags; any other
// nested line is parsed directly as a flags block. A non-matching "vendor" line at the top level
// is skipped for its single line only (so a later vendor clause in the same section can still be
// tried); reaching "applytoall", or the end of the file, without ever entering a matching vendor
// block returns true with the cursor left at that point.
uint8_t hwreq_parser_parse_vendor_block(hwreq_parser *self)
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
        if (_strnicmp((char *)self->cursor, "vendor", 6) == 0) {
            c = ((char *)self->cursor)[6];
            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                cursor = (char *)self->cursor + 6;
                while (*cursor == ' ' || *cursor == '\t') {
                    cursor++;
                }
                self->cursor = (uint32_t)cursor;

                if (*cursor == '=') {
                    cursor++;
                    self->cursor = (uint32_t)cursor;

                    if (_strnicmp(cursor, "unknown", 7) == 0 &&
                        (c = cursor[7], c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t')) {
                        vendor_id = (int32_t)self->adapter.vendor_id;
                    } else {
                        vendor_id = hwreq_token_parse_number(self);
                        if (vendor_id != (int32_t)self->adapter.vendor_id) {
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
                    msvc_string_assign_n(&self->graphics_vendor_name, name, length);

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

                        if (!(_strnicmp((char *)self->cursor, "vendor", 6) == 0 &&
                              (c = ((char *)self->cursor)[6],
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
                        if (_strnicmp((char *)self->cursor, "vendor", 6) == 0) {
                            c = ((char *)self->cursor)[6];
                            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                                return 1;
                            }
                        }
                        if (_strnicmp((char *)self->cursor, "unknown", 7) == 0) {
                            c = ((char *)self->cursor)[7];
                            if (c == '>' || c == '<' || c == '!' || c == '=' || c == ' ' || c == '\r' || c == '\t') {
                                device_id = (int32_t)self->adapter.device_id;
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
                            if (device_id == (int32_t)self->adapter.device_id) {
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
                                hwreq_string_assign_cstr(&self->graphics_device_name, name);
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
Original Ghidra decompilation (0x57a680):

bool FUN_0057a680(void)

{
  char cVar1;
  int in_EAX;
  int iVar2;
  char *pcVar3;
  char *pcVar4;

LAB_0057a690:
  iVar2 = __strnicmp(*(char **)(in_EAX + 8),"vendor",6);
  if (iVar2 == 0) {
    pcVar3 = (char *)(*(int *)(in_EAX + 8) + 6);
    cVar1 = *pcVar3;
    if ((((((cVar1 != '>') && (cVar1 != '<')) && (cVar1 != '!')) &&
         ((cVar1 != '=' && (cVar1 != ' ')))) && (cVar1 != '\r')) && (cVar1 != '\t'))
    goto LAB_0057a75a;
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
        iVar2 = *(int *)(in_EAX + 0x4e4);
      }
      else {
        iVar2 = hwreq_token_parse_number();
        if (iVar2 != *(int *)(in_EAX + 0x4e4)) goto LAB_0057a793;
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
LAB_0057a810:
      do {
        pcVar3 = *(char **)(in_EAX + 8);
        pcVar4 = pcVar3 + 1;
        *(char **)(in_EAX + 8) = pcVar4;
        if (*pcVar3 != '\r') {
          if (pcVar4 < *(char **)(in_EAX + 0xc)) goto LAB_0057a810;
        }
        if ((pcVar4 < *(char **)(in_EAX + 0xc)) && (*pcVar4 == '\n')) {
          *(char **)(in_EAX + 8) = pcVar3 + 2;
        }
        *(char **)(in_EAX + 0x10) = *(char **)(in_EAX + 8);
        *(int *)(in_EAX + 0x14) = *(int *)(in_EAX + 0x14) + 1;
        iVar2 = __strnicmp(*(char **)(in_EAX + 8),"vendor",6);
        if ((iVar2 != 0) ||
           ((((cVar1 = *(char *)(*(int *)(in_EAX + 8) + 6), cVar1 != '>' && (cVar1 != '<')) &&
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
            goto LAB_0057a8b1;
          }
          break;
        }
LAB_0057a8b1:
      } while (*(uint *)(in_EAX + 8) < *(uint *)(in_EAX + 0xc));
      do {
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
          iVar2 = *(int *)(in_EAX + 0x4e8);
          *(int *)(in_EAX + 8) = *(int *)(in_EAX + 8) + 7;
          hwreq_token_skip_whitespace();
LAB_0057a9e9:
          if (iVar2 == -1) {
            return true;
          }
          if (**(char **)(in_EAX + 8) == '=') {
            *(char **)(in_EAX + 8) = *(char **)(in_EAX + 8) + 1;
            iVar2 = hwreq_token_parse_quoted_string();
            if (iVar2 != 0) {
              FUN_0057b590(in_EAX + 0x40);
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
            (iVar2 = hwreq_token_parse_number(), iVar2 == *(int *)(in_EAX + 0x4e8))))
        goto LAB_0057a9e9;
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
LAB_0057a75a:
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
LAB_0057a793:
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
  goto LAB_0057a690;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
