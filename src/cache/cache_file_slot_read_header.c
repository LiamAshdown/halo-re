// cache_file_slot_read_header  (Ghidra: FUN_004435e0; renamed, see out/phase4/cache_types_notes.md
// "cache_file_header" section, which names this function directly)
// address 0x4435e0, size 385 bytes
// name confidence: 0.6   rewrite confidence: 0.70
// evidence: operates on &cache_file_slots[slot_index] at stride 0x80c (matches cache_file_slot
// in types/cache.h); the five-part header validation (head/foot signatures, file_size range,
// name strlen, version==7) is byte-identical to the other two validators cache_types_notes.md
// documents for cache_file_header. slot_index arrives in AX per the disassembly ("short in_AX").
// register convention: slot_index in EAX (low 16 bits only; in_AX).
//
// The async-IO path (os_platform >= 3) calls 0x442c70 / 0x442ce0, both outside this
// function's assigned address range and not rewritten here, but their register arguments are
// now recovered rather than inferred (objdump of 0x442c70-0x442d0c and of this function's own
// call site at 0x443657-0x44368a):
//   cache_io_read_file_ex_retry @0x442c70  stack (ReadFileEx, file, buffer) + ESI = a whole cache_io_request-shaped stack
//                 object whose OVERLAPPED half it zero-fills before storing EDX into
//                 OVERLAPPED.Offset, EBX = bytes to read (0x800), EDX = file offset (0),
//                 EDI = the APC (0x443b00, cache_io_completion_routine, which recovers the
//                 completion record from the same block at +0x24). It then calls ReadFileEx and
//                 retries through SleepEx on failure. So the ESI object is a cache_io_request,
//                 NOT a bare cache_io_completion: its completion sits at +0x24, which is exactly
//                 where this function writes {&header_read_ok, 0, 0} (0x443637/0x44363b/0x44363f
//                 land at frame+0x40 while ESI is frame+0x1c).
//   cache_io_wait_for_flag @0x442ce0       ESI = a plain uint8_t* flag, not a completion record: it is `cmp BYTE PTR
//                 [esi],0` / SleepEx(0x1388, alertable) until the byte is set or SleepEx stops
//                 returning WAIT_IO_COMPLETION (0xc0), and returns that byte in AL. Here it is
//                 handed &header_read_ok directly (`lea esi,[esp+0x13]` at 0x443681, the same
//                 slot 0x44368a reads back).
// The real prototypes of those two functions belong to whichever file rewrites them.
// reconciled: R10 profile_directory is char[0x105] (k_profile_directory_storage_size; shell zeroes 0x41 dwords + 1 byte at 0x540ef9)

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "cache.h"

extern cache_file_slot cache_file_slots[k_cache_file_slot_count]; // 0x006a9428
extern char profile_directory[0x105];                              // 0x006ac900
extern int32_t os_platform;                                        // 0x00721ef0
extern char *shell_fatal_error_argument;                           // 0x00722bbc

extern void os_platform_identify(void);                            // 0x5427e0
extern void shell_display_fatal_error_dialog(uint32_t string_id, uint32_t title_id, int32_t fatal); // 0x57ea70

extern void *ReadFileEx_exref;  // 0x0063a27c IAT slot holding ReadFileEx, passed by value

// blam-cc: request in ESI, size in EBX, offset in EDX, completion_routine in EDI (the three
// named stack arguments are all Ghidra shows); 0x442c70, outside this module
extern void cache_io_read_file_ex_retry(void *read_file_ex, void *file, void *buffer,
    cache_io_request *request, uint32_t size, uint32_t offset, void *completion_routine);
// blam-cc: flag in ESI; returns *flag in AL. 0x442ce0, outside this module
extern uint8_t cache_io_wait_for_flag(uint8_t *flag);
// The ReadFileEx APC FUN_00442c70 installs; declared only to take its address. 0x443b00
extern void cache_io_completion_routine(uint32_t error_code, uint32_t bytes_transferred,
    cache_io_request *overlapped);
extern uint8_t code_address_cache_io_completion_routine[]; // 0x00443b00: the original APC in the hooked build; in the
                                             // standalone build a __stdcall (ret 0xc) thunk into the C above

// blam-cc: slot_index in EAX (in_AX)
// Reads and validates one cache-file slot's 0x800-byte header from disk, synchronously
// (SetFilePointer + ReadFile) on platforms below os_platform 3, or via the async
// ReadFileEx-retry/wait pair otherwise. First stamps the slot's last-write time from the open
// file handle. On success the slot's header is left populated. If the header fails the five-part
// validation, the whole 0x800-byte header and the last-write time are zero-filled (quiet
// failure). If the read itself could not be performed at all, reports a fatal error and marks
// the slot's file handle invalid (-1).
void cache_file_slot_read_header(int32_t slot_index)
{
    cache_file_slot *slot;
    char path[256];
    uint8_t header_read_ok;
    uint32_t bytes_read;
    cache_io_request request;  // frame+0x1c; only its OVERLAPPED half and its completion at
                               // +0x24 are used, the rest is never touched here
    char *name_scan;

    slot = &cache_file_slots[slot_index];
    sprintf(path, "%s\\cache%03d.map", profile_directory, slot_index);

    GetFileTime(slot->file, (LPFILETIME)&slot->last_write_time, (void *)0, (void *)0);

    header_read_ok = 0;
    request.completion.flag = &header_read_ok;
    request.completion.procedure = (void (*)(cache_io_completion *))0;
    request.completion.data = (void *)0;

    if (os_platform == 0) {
        os_platform_identify();
    }

    if (os_platform < 3) {
        if (SetFilePointer(slot->file, 0, (void *)0, 0) != 0xffffffff) {
            if (ReadFile(slot->file, &slot->header, k_cache_file_header_size, &bytes_read, (void *)0) != 0 &&
                bytes_read == k_cache_file_header_size) {
                goto validate_header;
            }
        }
    } else {
        cache_io_read_file_ex_retry(ReadFileEx_exref, slot->file, &slot->header, &request,
            k_cache_file_header_size, 0, (void *)code_address_cache_io_completion_routine);
        // 0x443669: the APC is the ORIGINAL routine's address (mov edi,0x443b00). Windows calls it __stdcall
        // (ret 0xc); passing the cdecl C rewrite cache_io_completion_routine directly left the APC dispatcher
        // 12 bytes off and crashed at startup (EIP on the stack). 0x443b00 reaches the C rewrite through its
        // hook adapter, which does the ret 0xc.
        cache_io_wait_for_flag(&header_read_ok);
        if (header_read_ok != 0) {
validate_header:
            if (slot->header.head == k_cache_file_head_signature &&
                slot->header.foot == k_cache_file_foot_signature &&
                -1 < slot->header.file_size && slot->header.file_size < k_cache_file_maximum_size + 1) {
                name_scan = slot->header.name;
                while (*name_scan != '\0') {
                    name_scan++;
                }
                if ((uint32_t)(name_scan - slot->header.name) < k_cache_file_name_length &&
                    slot->header.version == k_cache_file_version) {
                    return;
                }
            }

            {
                uint8_t *zero;
                int32_t i;
                zero = (uint8_t *)&slot->header;
                for (i = 0x200; i != 0; i--) {
                    *(uint32_t *)zero = 0;
                    zero += 4;
                }
            }
            slot->last_write_time.low_date_time = 0;
            slot->last_write_time.high_date_time = 0;
            return;
        }
    }

    shell_fatal_error_argument = path;
    shell_display_fatal_error_dialog(0x89, 0x7e, 1);
    slot->file = (void *)0xffffffff;
    return;
}

#if 0
Original Ghidra decompilation (0x4435e0):

void FUN_004435e0(void)

{
  undefined4 *puVar1;
  short in_AX;
  int *piVar2;
  DWORD DVar3;
  BOOL BVar4;
  int iVar5;
  LPFILETIME lpCreationTime;
  int *piVar6;
  char local_139;
  LPFILETIME local_138;
  int *local_134;
  char *local_10c;
  undefined4 local_108;
  undefined4 local_104;
  char local_100 [256];

  iVar5 = in_AX * 0x80c;
  puVar1 = (undefined4 *)(&DAT_006a9428 + iVar5);
  _sprintf(local_100,"%s\\cache%03d.map",&DAT_006ac900,(int)in_AX);
  lpCreationTime = (LPFILETIME)(&DAT_006a942c + iVar5);
  local_138 = lpCreationTime;
  GetFileTime((HANDLE)*puVar1,lpCreationTime,(LPFILETIME)0x0,(LPFILETIME)0x0);
  local_10c = &local_139;
  local_139 = '\0';
  local_108 = 0;
  local_104 = 0;
  if (DAT_00721ef0 == 0) {
    os_platform_identify();
  }
  if (DAT_00721ef0 < 3) {
    DVar3 = SetFilePointer((HANDLE)*puVar1,0,(PLONG)0x0,0);
    if (DVar3 != 0xffffffff) {
      BVar4 = ReadFile((HANDLE)*puVar1,&DAT_006a9434 + iVar5,0x800,(LPDWORD)&local_138,
                       (LPOVERLAPPED)0x0);
      if ((BVar4 != 0) && (piVar6 = (int *)(&DAT_006a9434 + iVar5), local_138 == (LPFILETIME)0x800))
      goto LAB_004436a0;
    }
  }
  else {
    local_134 = (int *)(&DAT_006a9434 + iVar5);
    FUN_00442c70(ReadFileEx_exref,*puVar1,local_134);
    FUN_00442ce0();
    lpCreationTime = local_138;
    piVar6 = local_134;
    if (local_139 != '\0') {
LAB_004436a0:
      if ((((*piVar6 == 0x68656164) && (piVar6[0x1ff] == 0x666f6f74)) && (-1 < piVar6[2])) &&
         (piVar6[2] < 0x18000001)) {
        piVar2 = piVar6 + 8;
        do {
          iVar5 = *piVar2;
          piVar2 = (int *)((int)piVar2 + 1);
        } while ((char)iVar5 != '\0');
        if (((uint)((int)piVar2 - ((int)piVar6 + 0x21)) < 0x20) && (piVar6[1] == 7)) {
          return;
        }
      }
      for (iVar5 = 0x200; iVar5 != 0; iVar5 = iVar5 + -1) {
        *piVar6 = 0;
        piVar6 = piVar6 + 1;
      }
      lpCreationTime->dwLowDateTime = 0;
      lpCreationTime->dwHighDateTime = 0;
      return;
    }
  }
  DAT_00722bbc = local_100;
  shell_display_fatal_error_dialog(0x89,0x7e,1);
  *puVar1 = 0xffffffff;
  return;
}
#endif
