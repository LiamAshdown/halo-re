// sv_ban_penalty  (Ghidra: sv_ban_penalty, already named)
// address 0x4e3a80, size 589 bytes
// name confidence: 0.9   rewrite confidence: 0.65
// evidence: out/phase4/networking_functions.md; types/networking.h sv_ban_penalty_seconds[4]
// and k_network_ban_penalty_tiers; the literal duration-formatting strings.
// register convention: disassembly (objdump -d -M intel) shows argument_count tested directly
// via EAX with no stack read, while `arguments` is read from a genuine incoming stack slot
// (computed from the very first instructions, before any local push could have spilled a
// register there) -- unlike sv_ban.c/sv_kick.c in this same batch, which get their client
// argument through ECX. The "get/set console variable" commands in this batch (sv_ban_penalty,
// sv_tk_grace, sv_tk_cooldown, sv_banlist_file, sv_friendly_fire, sv_timelimit, sv_maxplayers,
// sv_rcon_password) all share this EAX/stack shape.
//   // blam-cc: EAX -> argument_count, stack -> arguments
// UNSURE: the trailing ">%1d         %s" / "Infinite" branch's exact reachability condition
// (`iVar6 == 4 && DAT_00699580 != -1`) is preserved literally though it looks specific to a
// table size of exactly 4 (k_network_ban_penalty_tiers).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdio.h>

extern int32_t sv_ban_penalty_seconds[4]; // 0x00699574
extern char sv_ban_penalty_arg_buffer[]; // 0x0066d6a4, UNSURE: scratch buffer reused by 0x4e51c0

extern int32_t parse_time_duration_string(char *string, char default_unit, uint8_t *unit_table); // this batch, 0x4e51c0
extern void chimera__console_out(const char *format, ...); // 0x496b50

// Console command: with no arguments, prints the current sv_ban_penalty escalating-duration
// table (up to 4 tiers, each formatted as "Nd Nh Nm Ns", "---" for zero, or "Infinite" past the
// table's end); with 1-4 arguments, parses each as a d/h/m/s duration, stopping (and leaving any
// remaining tiers at their previous values) the moment one argument parses to exactly 0, which
// instead marks that one tier indefinite (-1); any other parse failure restores the whole table
// to what it was before the command ran.
void sv_ban_penalty(uint32_t argument_count, int32_t *arguments) // blam-cc: EAX -> argument_count, stack -> arguments
{
    int32_t saved[4];
    int32_t i;

    if (argument_count == 0) {
    print_table:
        chimera__console_out("Offense    %s", "Ban Time");
        i = 0;
        do {
            int32_t seconds = sv_ban_penalty_seconds[i];
            char buf[16];
            if (seconds == -1) {
                chimera__console_out("%1d          %s", i + 1, "Infinite");
                break;
            }
            {
                int32_t days = (seconds % 0x15180) / 0xe10;
                int32_t rem = (seconds % 0x15180) % 0xe10;
                int32_t hours = rem / 0x3c;
                rem = rem % 0x3c;
                if (seconds / 0x15180 == 0) {
                    if (days == 0) {
                        if (hours == 0) {
                            if (rem == 0) {
                                sprintf(buf, "---");
                            } else {
                                sprintf(buf, "%ds", rem);
                            }
                        } else {
                            sprintf(buf, "%dm %ds", hours, rem);
                        }
                    } else {
                        sprintf(buf, "%dh %dm %ds", days, hours, rem);
                    }
                } else {
                    sprintf(buf, "%dd %dh %dm %ds", seconds / 0x15180, days, hours, rem);
                }
            }
            i = i + 1;
            chimera__console_out("%1d          %s", i, buf);
        } while (i < 4);
        if (i == 4 && sv_ban_penalty_seconds[3] != -1) {
            chimera__console_out(">%1d         %s", 4, "Infinite");
        }
        return;
    }
    if (0 < (int32_t)argument_count && (int32_t)argument_count < 5) {
        saved[0] = sv_ban_penalty_seconds[0];
        saved[1] = sv_ban_penalty_seconds[1];
        saved[2] = sv_ban_penalty_seconds[2];
        saved[3] = sv_ban_penalty_seconds[3];
        i = 0;
        do {
            int32_t value = parse_time_duration_string((char *)arguments[i], 'm', (uint8_t *)sv_ban_penalty_arg_buffer);
            if (value == 0) {
                sv_ban_penalty_seconds[i] = -1;
                break;
            }
            if (value < 1) {
                sv_ban_penalty_seconds[0] = saved[0];
                sv_ban_penalty_seconds[1] = saved[1];
                sv_ban_penalty_seconds[2] = saved[2];
                sv_ban_penalty_seconds[3] = saved[3];
                chimera__console_out("Incorrect usage. Type help sv_ban_penalty for more information.");
                return;
            }
            sv_ban_penalty_seconds[i] = value;
            i = i + 1;
        } while (i < (int32_t)argument_count);
        goto print_table;
    }
    chimera__console_out("Incorrect usage. Type help sv_ban_penalty for more information.");
}

#if 0
Original Ghidra decompilation (0x4e3a80), from tools/pack.py 0x4e3a80:

void sv_ban_penalty(void)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  int in_EAX;
  int iVar4;
  int iVar5;
  int iVar6;
  char local_20 [4];
  undefined4 local_1c;
  undefined4 local_18;
  int local_14;

  uVar3 = DAT_00699574;
  if (in_EAX == 0) {
LAB_004e3b10:
    chimera__console_out("Offense    %s","Ban Time");
    iVar6 = 0;
    do {
      iVar4 = (&DAT_00699574)[iVar6];
      if (iVar4 == -1) {
        chimera__console_out("%1d          %s",iVar6 + 1,"Infinite");
        break;
      }
      iVar1 = (iVar4 % 0x15180) / 0xe10;
      iVar5 = (iVar4 % 0x15180) % 0xe10;
      iVar2 = iVar5 / 0x3c;
      iVar5 = iVar5 % 0x3c;
      if (iVar4 / 0x15180 == 0) {
        if (iVar1 == 0) {
          if (iVar2 == 0) {
            if (iVar5 == 0) {
              _sprintf(local_20,"---");
            }
            else {
              _sprintf(local_20,(char *)&PTR_DAT_0066d5c4,iVar5);
            }
          }
          else {
            _sprintf(local_20,"%dm %ds",iVar2,iVar5);
          }
        }
        else {
          _sprintf(local_20,"%dh %dm %ds",iVar1,iVar2,iVar5);
        }
      }
      else {
        _sprintf(local_20,"%dd %dh %dm %ds",iVar4 / 0x15180,iVar1,iVar2,iVar5);
      }
      iVar6 = iVar6 + 1;
      chimera__console_out("%1d          %s",iVar6,local_20);
    } while (iVar6 < 4);
    if ((iVar6 == 4) && (DAT_00699580 != -1)) {
      chimera__console_out(">%1d         %s",4,"Infinite");
      return;
    }
  }
  else {
    if ((0 < in_EAX) && (in_EAX < 5)) {
      iVar6 = 0;
      local_1c = DAT_00699578;
      local_18 = DAT_0069957c;
      local_14 = DAT_00699580;
      if (0 < in_EAX) {
        do {
          iVar4 = parse_time_duration_string(0x6d,&DAT_0066d6a4);
          if (iVar4 == 0) {
            (&DAT_00699574)[iVar6] = 0xffffffff;
            break;
          }
          if (iVar4 < 1) {
            DAT_00699578 = local_1c;
            DAT_0069957c = local_18;
            DAT_00699580 = local_14;
            goto LAB_004e3be6;
          }
          (&DAT_00699574)[iVar6] = iVar4;
          iVar6 = iVar6 + 1;
        } while (iVar6 < in_EAX);
        goto LAB_004e3b10;
      }
    }
LAB_004e3be6:
    DAT_00699574 = uVar3;
    chimera__console_out("Incorrect usage. Type help sv_ban_penalty for more information.");
  }
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe) confirms the register convention:
  4e3a80: sub esp,0x20
  4e3a83: push ebx
  4e3a84: push ebp
  4e3a85: push esi
  4e3a86: push edi
  4e3a87: mov edi,eax           ; edi = argument_count
  4e3a89: test edi,edi
  4e3ad0: mov ebx,[esp+0x34]    ; ebx = arguments (genuine stack argument: 4 pushes deep, so
                                ; [esp+0x34] lands exactly on the caller's pushed slot)
#endif
