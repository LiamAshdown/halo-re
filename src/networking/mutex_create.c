// mutex_create  (Ghidra: mutex_create, already named)
// address 0x440510, size 89 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: out/phase4/networking_types_notes.md "network_mutex_record (0x28) /
// network_thread_record (0x08)": `mutex_create` snprintfs "mutex_%ld" into slot+0x04 with a
// 0x20 limit and stores the CreateMutexA handle at slot+0x00; string "mutex_%ld" confirms the
// naming hint.
// register convention: out-handle pointer in unaff_EDI (this function takes no recognized
// stack/EAX/ECX/EDX arguments), mapped to EDI per the register order.
//   // blam-cc: EDI -> out_handle
// FIXED (register inputs, objdump): EDI carries out_handle (read at 0x440552, mov [edi],esi);
// the note used "out-handle" (hyphen) where the parameter is named out_handle (underscore), so
// the checker's alias match failed even though the code already used it correctly.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_mutex_name_counter; // 0x006f0cac

extern network_mutex_record *network_mutex_slot_allocate(void); // 0x440420, this module
extern int32_t snprintf(char *buffer, uint32_t count, const char *format, ...);

// blam-cc: EDI -> out_handle
int32_t mutex_create(network_mutex_record **out_handle)
{
    network_mutex_record *slot;
    int32_t name_index;

    slot = network_mutex_slot_allocate();
    name_index = network_mutex_name_counter;
    if (slot == 0) {
        *out_handle = 0;
        return 0;
    }
    network_mutex_name_counter = network_mutex_name_counter + 1;
    snprintf(slot->name, 0x20, "mutex_%ld", name_index);
    slot->handle = CreateMutexA(0, 0, 0);
    if (slot->handle != 0) {
        *out_handle = slot;
        return 1;
    }
    *out_handle = 0;
    return 0;
}

#if 0
Original Ghidra decompilation (0x440510):

undefined4 mutex_create(void)

{
  int iVar1;
  undefined4 *puVar2;
  HANDLE pvVar3;
  undefined4 *unaff_EDI;

  puVar2 = network_mutex_slot_allocate();
  iVar1 = DAT_006f0cac;
  if (puVar2 == (undefined4 *)0x0) {
    *unaff_EDI = 0;
    return 0;
  }
  DAT_006f0cac = DAT_006f0cac + 1;
  __snprintf((char *)(puVar2 + 1),0x20,"mutex_%ld",iVar1);
  pvVar3 = CreateMutexA((LPSECURITY_ATTRIBUTES)0x0,0,(LPCSTR)0x0);
  *puVar2 = pvVar3;
  if (pvVar3 != (HANDLE)0x0) {
    *unaff_EDI = puVar2;
    return 1;
  }
  *unaff_EDI = 0;
  return 0;
}
#endif
