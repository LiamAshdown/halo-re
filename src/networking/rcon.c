// rcon  (Ghidra: rcon, already named)
// address 0x4e4c00, size 448 bytes
// name confidence: 0.85   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md ("packages a password and a re-quoted command
// line and sends it to the server as an rcon request"); disassembly (objdump -d -M intel) pins
// both the register convention and the exact quoting rule: the first extra argument (the rcon
// command itself) is copied bare, and every argument after it is individually wrapped in a
// literal `"..."` pair, all space-separated, into a 64-character command buffer whose remaining
// budget is charged 3 + strlen(word) per word (2 quote characters + 1 separator, whether or not
// this particular word is actually quoted).
// register convention: both arguments arrive on the stack (Ghidra recognizes both as ordinary
// parameters); confirmed by disassembly reading them from genuine incoming stack slots with no
// prior register spill.
//   // blam-cc: stack -> argument_count, stack -> arguments
// UNSURE: 0x0066cfe4/0x0066d01c/0x0066d050/0x0066d1c/0x0066cfe4-family string addresses for the
// error messages are taken from the strings-referenced list, not individually matched to each
// branch by address; the mapping used here follows the order they appear in Ghidra's own
// decompiled control flow, which this rewrite otherwise preserves exactly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern int16_t network_game_mode; // 0x00719720 (types/game.h), 1 == client

extern void rcon_send_request(char *command, char *password); // this batch, 0x4e4dc0
extern void *console_color_006851fc; // 0x006851fc, a ColorARGB * the original loads into EAX
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Console command: packages the password (first argument) and the remaining arguments -- the
// rcon command bare, every argument after it individually quoted -- into one command line, and
// sends it to the server via rcon_send_request. Client-only; refuses on a dedicated/listen
// server, on fewer than 2 arguments, on a password outside 1-8 characters, or if the rebuilt
// command line would exceed 64 characters.
void rcon(int32_t argument_count, char **arguments) // blam-cc: stack -> argument_count, stack -> arguments
{
    char *password;
    int32_t password_len;
    char command[68];
    int32_t budget;
    int32_t i;

    if (network_game_mode != 1) {
        chimera__console_out((ColorARGB *)console_color_006851fc, "rcon is a client-only function!");
        return;
    }
    if (argument_count < 2) {
        chimera__console_out((ColorARGB *)console_color_006851fc, "Incorrect usage. Type help rcon for more information.");
        return;
    }
    password = arguments[0];
    password_len = strlen(password);
    if (password_len == 0 || 8 < password_len) {
        chimera__console_out((ColorARGB *)console_color_006851fc, "rcon password must be between 1 and %d characters", 8);
        return;
    }

    command[0] = 0;
    budget = 0x40;
    for (i = 1; i < argument_count; i = i + 1) {
        char *word = arguments[i];
        int32_t word_len = strlen(word);

        budget = budget + (-3 - word_len);
        if (budget < 0) {
            chimera__console_out((ColorARGB *)console_color_006851fc, "rcon command can be no longer than %d characters", 0x40);
            return;
        }
        if (command[0] != 0) {
            strcat(command, " ");
        }
        if (1 < i) {
            strcat(command, "\"");
        }
        strcat(command, word);
        if (1 < i) {
            strcat(command, "\"");
        }
    }
    rcon_send_request(command, password);
}

#if 0
Original Ghidra decompilation (0x4e4c00), from tools/pack.py 0x4e4c00:

void rcon(int param_1,undefined4 *param_2)

{
  char *pcVar1;
  char cVar2;
  char *pcVar3;
  char *pcVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  undefined2 *puVar9;
  undefined1 local_48 [4];
  char local_44 [68];

  iVar8 = 1;
  if (DAT_00719720 != 1) {
    chimera__console_out("rcon is a client-only function!");
    return;
  }
  if (param_1 < 2) {
    chimera__console_out("Incorrect usage. Type help rcon for more information.");
    return;
  }
  pcVar4 = (char *)*param_2;
  pcVar3 = pcVar4;
  do {
    cVar2 = *pcVar3;
    pcVar3 = pcVar3 + 1;
  } while (cVar2 != '\0');
  local_48 = (undefined1  [4])((int)pcVar3 - (int)(pcVar4 + 1));
  if (local_48 != (undefined1  [4])0x0) {
    pcVar3 = pcVar4 + 1;
    do {
      cVar2 = *pcVar4;
      pcVar4 = pcVar4 + 1;
    } while (cVar2 != '\0');
    if ((uint)((int)pcVar4 - (int)pcVar3) < 9) {
      pcVar4 = local_44;
      for (iVar6 = 0x10; iVar6 != 0; iVar6 = iVar6 + -1) {
        pcVar4[0] = '\0'; pcVar4[1] = '\0'; pcVar4[2] = '\0'; pcVar4[3] = '\0';
        pcVar4 = pcVar4 + 4;
      }
      *pcVar4 = '\0';
      iVar6 = 0x40;
      if (1 < param_1) {
        do {
          pcVar4 = (char *)param_2[iVar8];
          pcVar3 = pcVar4;
          do { cVar2 = *pcVar3; pcVar3 = pcVar3 + 1; } while (cVar2 != '\0');
          iVar6 = iVar6 + (-3 - ((int)pcVar3 - (int)(pcVar4 + 1)));
          if (iVar6 < 0) {
            chimera__console_out("rcon command can be no longer than %d characters",0x40);
            return;
          }
          pcVar3 = local_44;
          do { cVar2 = *pcVar3; pcVar3 = pcVar3 + 1; } while (cVar2 != '\0');
          local_48 = (undefined1  [4])((int)pcVar3 - (int)(local_44 + 1));
          if (local_48 != (undefined1  [4])0x0) {
            /* append ' ' to local_44 */
          }
          if (1 < iVar8) {
            /* append '"' to local_44 */
          }
          /* append pcVar4 (the word) to local_44 */
          if (1 < iVar8) {
            /* append '"' to local_44 */
          }
          iVar8 = iVar8 + 1;
        } while (iVar8 < param_1);
      }
      rcon_send_request();
      return;
    }
  }
  chimera__console_out("rcon password must be between 1 and %d characters",8);
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe) resolved the append blocks Ghidra's own decompile
collapsed into raw byte copies, and confirmed argc/argv both arrive on the stack:
  4e4c16: cmp DWORD PTR [esp+0x54],0x2   ; argc, a genuine stack slot (zero pushes so far)
  4e4c21: mov eax,[esp+0x58]             ; argv, likewise
  4e4ccc: mov ax,ds:0x660788             ; the space separator (2 bytes: ' ' + NUL)
  4e4cd2: mov [edi],ax
  4e4ce8: mov cx,ds:0x665120             ; the quote character (2 bytes: '"' + NUL)
  4e4cef: mov [edi],cx
  4e4d45: mov esi,[esp+0x10]             ; password, reloaded from its saved slot
  4e4d49: lea eax,[esp+0x18]             ; command buffer
  4e4d4f: call rcon_send_request         ; EAX -> command, ECX -> password (see that file)
#endif
