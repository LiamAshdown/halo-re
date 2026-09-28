// random_seed_generate  (Ghidra: random_seed_generate, already named)
// address 0x4cd070, size 110 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: out/phase4/math_types_notes.md "globals owned elsewhere" (0x006ac8f8/0x006ac8fc,
//   the 64-bit QueryPerformanceFrequency result) and misattributed-functions note 4: "mostly
//   platform code ... its only math-module output is the seed", consumed by
//   sphere_point_table_init @0x4cd0e0 to seed local_random_seed.
// register convention: __cdecl, no arguments.

#include "win32.h"
#include "tags.h"
#include "math.h"

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc, QueryPerformanceFrequency() result, owned by the timing/system module

extern int rand(void); // 0x006240cf _rand

// Produces a hard-to-predict 32-bit seed by XOR-combining rand() with two independently timed
// QueryPerformanceCounter readings, each converted to a frequency-scaled ratio (one of them
// additionally scaled by 1000 first). __allmul/__alldiv in the original are just the MSVC 7.1
// 64-bit multiply/divide helpers; folded here into plain int64_t arithmetic with identical
// results.
uint32_t random_seed_generate(void)
{
    large_integer counter_a;
    large_integer counter_b;
    uint32_t rand_value;
    uint32_t scaled_a;
    uint32_t scaled_b;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter_a);
    QueryPerformanceCounter((LARGE_INTEGER *)&counter_b);
    rand_value = (uint32_t)rand();

    scaled_a = (uint32_t)((counter_a.quad_part * 1000) / performance_frequency);
    scaled_b = (uint32_t)(counter_b.quad_part / performance_frequency);

    return rand_value ^ scaled_a ^ scaled_b;
}

#if 0
Original Ghidra decompilation (0x4cd070):

uint __cdecl random_seed_generate(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  undefined8 uVar6;
  LARGE_INTEGER local_10;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_10);
  QueryPerformanceCounter(&local_8);
  uVar3 = _rand();
  uVar6 = __allmul(local_10.s.LowPart,local_10.s.HighPart,1000,0);
  uVar2 = DAT_006ac8fc;
  uVar1 = DAT_006ac8f8;
  uVar4 = __alldiv(uVar6,DAT_006ac8f8,DAT_006ac8fc);
  uVar5 = __alldiv(local_8.s.LowPart,local_8.s.HighPart,uVar1,uVar2);
  return uVar3 ^ uVar4 ^ uVar5;
}
#endif
