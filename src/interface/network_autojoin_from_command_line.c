// network_autojoin_from_command_line  (Ghidra: FUN_004c9c80, named in phase 4)
// address 0x4c9c80, size 334 bytes (0x2220 byte frame through _chkstk 0x628240)
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: rewritten from objdump 0x4c9c80..0x4c9dcd in the phase-4 review. The first
// rewrite mixed up the out locals of command_line_check_flag (EDI is its only output, the
// argument value, zeroed first), used the wrong count (EBX in/out of 0x53c4e0), compared the
// name at the wrong offset, broke out of the loop on the default slot and loaded the profile
// with the wrong arguments.
//   Returns 0 unless -connect (0x0066b2ec) is on the command line with a value. With -name
// (0x0066b2e4) and a value, the name is widened into a 0x80 byte buffer (0x557990: EBX
// source, EAX destination, EDI size) and up to 100 saved profiles of type 0 are enumerated
// (0x53c4e0, EBX in/out count). They are walked from the last one down; slot -1 copies the
// default profile (0x0071d280, 0x1ffc bytes) into the local profile and moves on, other
// slots are read with player_profile_get (ECX profile) and the first whose name (+2)
// matches with wcscmp (0x627d17, case sensitive) is loaded for local player 0
// (player_profile_load, AX player, EDX profile, stack slot). Then the game connects to the
// -connect address with the -password (0x0066b2d8) value, or "" (0x0065512c) without one
// (network_game_client_connect_to_address_async), and 1 is returned.
// register convention: plain cdecl, no arguments; returns AL.

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern uint8_t default_profile_data[0x1ffc]; // 0x0071d280, UNSURE name
extern char empty_string_0065512c[];                  // 0x0065512c

extern uint8_t command_line_check_flag(const char *flag, const char **out_value); // 0x542760, blam-cc: EDI out_value (zeroed, then the argument after the flag)
extern uint16_t *string_convert_ascii_to_unicode(uint16_t *dest, int32_t dest_bytes, const char *source); // 0x557990, blam-cc: EAX dest, EDI dest_bytes, EBX source; 8-bit to wide copy
extern void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only,
    uint16_t *capacity_and_count); // 0x53c4e0, stack (type, out, builtin_only), EBX &count
extern uint8_t player_profile_get(int32_t slot, void *out_profile); // 0x53a770; blam-cc: ECX -> out_profile
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id); // 0x495970, blam-cc: AX player_index, EDX source_profile
extern int32_t _wcscmp(const uint16_t *a, const uint16_t *b); // 0x627d17, CRT
extern uint8_t network_game_client_connect_to_address_async(const char *address, const char *password); // 0x4c8500

uint8_t network_autojoin_from_command_line(void)
{
    const char *address = 0;
    const char *name = 0;
    const char *password = 0;
    uint16_t wide_name[0x40];
    int32_t slots[100];
    uint8_t profile[0x1ffc];

    if (!command_line_check_flag("-connect", &address) || address == 0) {
        return 0;
    }

    if (command_line_check_flag("-name", &name) && name != 0) {
        int16_t count = 100;

        string_convert_ascii_to_unicode(wide_name, 0x80, name);
        saved_game_enumerate_by_type(0, slots, 0, &count);
        while (count > 0) {
            int32_t slot = slots[count - 1];

            if (slot == -1) {
                memcpy(profile, default_profile_data, sizeof(profile));
            } else if (player_profile_get(slot, profile) != 0 &&
                       _wcscmp(wide_name, (const uint16_t *)(profile + 2)) == 0) {
                player_profile_load(0, profile, slot);
                break;
            }
            count--;
        }
    }

    if (!command_line_check_flag("-password", &password) || password == 0) {
        password = empty_string_0065512c;
    }
    network_game_client_connect_to_address_async(address, password);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4c9c80) -- badly broken (Ghidra removed most blocks as
"unreachable"); the rewrite above follows objdump -d 0x4c9c80..0x4c9dcb instead (see below):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Removing unreachable block (ram,0x004c9cc9) */
/* WARNING: Removing unreachable block (ram,0x004c9ce2) */
/* WARNING: Removing unreachable block (ram,0x004c9cee) */
/* WARNING: Removing unreachable block (ram,0x004c9d23) */
/* WARNING: Removing unreachable block (ram,0x004c9d47) */
/* WARNING: Removing unreachable block (ram,0x004c9d5b) */
/* WARNING: Removing unreachable block (ram,0x004c9d7c) */
/* WARNING: Removing unreachable block (ram,0x004c9d32) */
/* WARNING: Removing unreachable block (ram,0x004c9d43) */
/* WARNING: Removing unreachable block (ram,0x004c9d45) */
/* WARNING: Removing unreachable block (ram,0x004c9d74) */
/* WARNING: Removing unreachable block (ram,0x004c9d7a) */
/* WARNING: Removing unreachable block (ram,0x004c9d8e) */
/* WARNING: Removing unreachable block (ram,0x004c9da3) */
/* WARNING: Removing unreachable block (ram,0x004c9dab) */
/* WARNING: Removing unreachable block (ram,0x004c9db0) */

undefined4 FUN_004c9c80(void)

{
  command_line_check_flag("-connect");
  return 0;
}

Disassembly (objdump, 0x4c9c80..0x4c9dcb), which the rewrite above actually follows:

004c9c80:
  55                   push   ebp
  8b ec                mov    ebp,esp
  83 e4 f8             and    esp,0xfffffff8
  b8 20220000          mov    eax,0x2220
  e8 b0e51500          call   0x628240                 ; alloca_probe
  53                   push   ebx
  55                   push   ebp
  56                   push   esi
  57                   push   edi
  33 f6                xor    esi,esi
  68 ecb26600          push   0x66b2ec                    ; "-connect"
  8d 7c 24 20          lea    edi,[esp+0x20]
  32 db                xor    bl,bl
  89 74 24 20          mov    [esp+0x20],esi
  89 74 24 18          mov    [esp+0x18],esi
  89 74 24 1c          mov    [esp+0x1c],esi
  e8 ae8a0700          call   0x542760                   ; command_line_check_flag
  83 c4 04             add    esp,0x4
  84 c0                test   al,al
  0f 84 07010000       je     0x4c9dc4                     ; not found: return false
  8b 6c 24 1c          mov    ebp,[esp+0x1c]                 ; ebp = connect target string
  3b ee                cmp    ebp,esi
  0f 84 fb000000       je     0x4c9dc4                        ; empty: return false
  68 e4b26600          push   0x66b2e4                          ; "-name"
  8d 7c 24 18          lea    edi,[esp+0x18]
  e8 898a0700          call   0x542760
  83 c4 04             add    esp,0x4
  84 c0                test   al,al
  0f 84 ac000000       je     0x4c9d8e                          ; no -name: skip to password
  8b 5c 24 14          mov    ebx,[esp+0x14]
  3b de                cmp    ebx,esi
  0f 84 a0000000       je     0x4c9d8e
  bf 80000000          mov    edi,0x80
  8d 44 24 20          lea    eax,[esp+0x20]
  c7 44 24 14 64000000 mov    DWORD PTR [esp+0x14],0x64
  e8 8cdc0800          call   0x557990
  56                   push   esi
  8d 84 24 a4000000    lea    eax,[esp+0xa4]
  50                   push   eax
  56                   push   esi
  8d 5c 24 20          lea    ebx,[esp+0x20]
  e8 c9270700          call   0x53c4e0                        ; saved_game_enumerate_by_type
  8b 5c 24 20          mov    ebx,[esp+0x20]
  83 c4 0c             add    esp,0xc
  66 85 db             test   bx,bx
  7e 6b                jle    0x4c9d8e
  0f bf cb             movsx  ecx,bx
  8b b4 8c 9c000000    mov    esi,[esp+ecx*4+0x9c]
  83 fe ff             cmp    esi,0xffffffff
  75 15                jne    0x4c9d47
  b9 ff070000          mov    ecx,0x7ff
  be 80d27100          mov    esi,0x71d280
  8d bc 24 30020000    lea    edi,[esp+0x230]
  f3 a5                rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
  eb 2d                jmp    0x4c9d74
  56                   push   esi
  8d 8c 24 34020000    lea    ecx,[esp+0x234]
  e8 1c0a0700          call   0x53a770                       ; player_profile_get
  83 c4 04             add    esp,0x4
  84 c0                test   al,al
  74 19                je     0x4c9d74
  8d 94 24 32020000    lea    edx,[esp+0x232]
  52                   push   edx
  8d 44 24 24          lea    eax,[esp+0x24]
  50                   push   eax
  e8 aadf1500          call   0x627d17                       ; _wcscmp
  83 c4 08             add    esp,0x8
  85 c0                test   eax,eax
  74 08                je     0x4c9d7c
  4b                   dec    ebx
  66 85 db             test   bx,bx
  7f a9                jg     0x4c9d23
  eb 12                jmp    0x4c9d8e
  56                   push   esi
  8d 94 24 34020000    lea    edx,[esp+0x234]
  33 c0                xor    eax,eax
  e8 e5bbfcff          call   0x495970                       ; player_profile_load
  83 c4 04             add    esp,0x4
  4c9d8e:
  68 d8b26600          push   0x66b2d8                        ; "-password"
  8d 7c 24 1c          lea    edi,[esp+0x1c]
  e8 c4890700          call   0x542760
  83 c4 04             add    esp,0x4
  84 c0                test   al,al
  74 08                je     0x4c9dab
  8b 44 24 18          mov    eax,[esp+0x18]
  85 c0                test   eax,eax
  75 05                jne    0x4c9db0
  4c9dab:
  b8 2c516500          mov    eax,0x65512c                    ; default password string
  4c9db0:
  50                   push   eax
  55                   push   ebp
  e8 49e7ffff          call   0x4c8500                        ; network_game_client_connect_to_address_async
  83 c4 08             add    esp,0x8
  b0 01                mov    al,0x1
  5f                   pop    edi
  5e                   pop    esi
  5d                   pop    ebp
  5b                   pop    ebx
  8b e5                mov    esp,ebp
  5d                   pop    ebp
  c3                   ret
  4c9dc4:
  5f                   pop    edi
  8a c3                mov    al,bl
  5e                   pop    esi
  5d                   pop    ebp
  5b                   pop    ebx
  8b e5                mov    esp,ebp
  5d                   pop    ebp
  c3                   ret
#endif
