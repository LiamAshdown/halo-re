#include "tags.h"
#include "memory.h"
#include "cseries.h"

#define CHECK(name, cond) typedef char check_##name[(cond) ? 1 : -1]

// constants the code compares against or passes
CHECK(path_len, k_cseries_path_length == 0x104);
CHECK(dir_buf, k_directory_create_buffer_size == 0x100);
CHECK(profile_storage, k_profile_directory_storage_size == 0x41 * 4 + 1);
CHECK(ms, k_milliseconds_per_second == 0x3e8);
CHECK(cutoff, k_qsort_dword_shortsort_cutoff == 8);
// two pending-partition stacks, 0x78 bytes each, fill the 0xf4 frame after the counter
CHECK(stack_depth, k_qsort_dword_stack_depth * 4 == 0x8c - 0x14);
CHECK(stack_frame, 4 + 2 * k_qsort_dword_stack_depth * 4 == 0xf4);
CHECK(level, k_error_file_minimum_level == 2);
CHECK(csidl, k_csidl_personal == 5);
CHECK(sem, k_sem_noopenfileerrorbox == 0x8000);
CHECK(dialog, k_profile_path_error_title == 0x8b && k_profile_path_error_message == 0x8c);
// profile directory runs up to the end of the zeroed block
CHECK(profile_end, 0x006ac900 + k_profile_directory_storage_size == 0x006aca05);
CHECK(freq_end, 0x006ac8f8 + 8 == 0x006ac900);

static uint8_t greater_or_equal(int32_t a, int32_t b) { return (uint8_t)(a >= b); }
int main(void) { qsort_dword_compare_proc p = greater_or_equal; (void)p; return 0; }
