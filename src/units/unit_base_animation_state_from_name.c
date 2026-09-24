// unit_base_animation_state_from_name  (Ghidra: FUN_0056eb90; renamed for this rewrite)
// address 0x56eb90, size 52 bytes
// name confidence: 0.7   rewrite confidence: 0.75
// evidence: out/phase4/units_types_notes.md "0x56eb90 -- a six-string table lookup against
//   0x0069fde4; it is the inverse of unit_base_animation_state and needs no type of its own."
//   0x0069fde4 is `unit_base_animation_state_names[6]`, established in
//   src/units/unit_set_or_test_seat_and_weapon_label.c and reused by
//   src/units/unit_get_seat_or_state_name.c (the forward direction: index -> name). This
//   function is the reverse: name -> index, case-insensitively, returning -1
//   (`_unit_base_animation_state_none`) when nothing matches. types/units.h already documents
//   `k_unit_base_animation_state_count = 6` against the same table for this exact reason.
// register convention: name pointer in EDI (unaff_EDI); no stack arguments.
//   // blam-cc: EDI -> name

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern char *unit_base_animation_state_names[6]; // 0x0069fde4
extern int __stricmp(const char *a, const char *b); // 0x628d8b, MSVC 7.1 CRT

// Case-insensitively matches `name` against unit_base_animation_state_names and returns the
// matching unit_base_animation_state, or _unit_base_animation_state_none if none matches.
int16_t unit_base_animation_state_from_name(const char *name)
{
    int16_t index;

    for (index = 0; index < 6; index++) {
        if (__stricmp(name, unit_base_animation_state_names[index]) == 0) {
            return index;
        }
    }
    return (int16_t)_unit_base_animation_state_none;
}

#if 0
Original Ghidra decompilation (0x56eb90):

short FUN_0056eb90(void)

{
  int iVar1;
  short sVar2;
  char *unaff_EDI;

  sVar2 = 0;
  do {
    iVar1 = __stricmp(unaff_EDI,(&PTR_DAT_0069fde4)[sVar2]);
    if (iVar1 == 0) {
      return sVar2;
    }
    sVar2 = sVar2 + 1;
  } while (sVar2 < 6);
  return -1;
}
#endif
