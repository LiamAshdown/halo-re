// autopatch_temp_name_generate  (Ghidra: FUN_00575fa0; named per this rewrite)
// address 0x575fa0, size 68 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary: "Seeds the RNG from the system clock and
// writes a random 7-letter lowercase string into a static buffer, likely used as a temporary
// file name for downloads."
// register convention: no register-passed arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t autopatch_temp_name_buffer[11]; // 0x006ef910, bytes 0..10
extern uint8_t autopatch_temp_name_flag;       // 0x006ef91b, UNSURE: cleared here, adjacent to the buffer
extern void srand(unsigned int seed); // CRT srand (0x6240c2: stores the seed in the per-thread data, _getptd()->_holdrand)
extern int32_t rand(void);
extern int32_t __time32(void *unused);

// Seeds the CRT RNG from the current time (xored with a constant) and writes a random 7-letter
// lowercase string into the shared temp-name buffer, used as a scratch download file name.
char *autopatch_temp_name_generate(void)
{
    uint32_t now;
    int32_t i;

    now = (uint32_t)__time32(0);
    srand(now ^ 0x33333333);
    autopatch_temp_name_flag = 0;
    for (i = 10; i >= 4; i--) {
        autopatch_temp_name_buffer[i] = (uint8_t)(rand() % 0x1a) + 'a';
    }
    return (char *)&autopatch_temp_name_buffer[4]; // 0x006ef914
}

#if 0
Original Ghidra decompilation (0x575fa0):

undefined * FUN_00575fa0(void)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;

  uVar1 = FID_conflict___time32((__time32_t *)0x0);
  FUN_006240c2(uVar1 ^ 0x33333333);
  DAT_006ef91b = 0;
  iVar4 = 7;
  do {
    iVar3 = iVar4 + -1;
    iVar2 = _rand();
    *(char *)((int)&DAT_006ef910 + iVar4 + 3) = (char)(iVar2 % 0x1a) + 'a';
    iVar4 = iVar3;
  } while (iVar3 != 0);
  return &DAT_006ef914;
}
#endif
