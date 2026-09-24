# cseries module: 9 functions (address, current Ghidra name, size, agent confidence, summary)

- 0x4491e0 string_to_lowercase size=36 conf=0.6 :: Lowercases a null-terminated string in place using the CRT tolower().
- 0x449210 FUN_00449210 size=59 conf=0.4 :: Reads the high-resolution performance counter and converts it to milliseconds using a previously cached counter frequency.
- 0x449250 directory_create_recursive size=278 conf=0.8 :: Creates a directory and all of its missing parent directories, correctly skipping the server/share components of UNC paths, returning whether the full path exists or was successfully created.
- 0x449370 memory_global_alloc size=10 conf=0.5 :: Thin wrapper allocating a block via the Win32 GlobalAlloc API.
- 0x449380 memory_global_free size=8 conf=0.5 :: Thin wrapper freeing a block via the Win32 GlobalFree API.
- 0x449390 profile_path_initialize size=185 conf=0.6 :: Determines and stores the player's profile directory path, honoring a -path command-line override or defaulting to "<Documents>\My Games\Halo", with a fatal-error fallback if the special-folder lookup
- 0x449450 write_to_error_file size=314 conf=0.85 :: Appends a (optionally timestamped) message string to the game's debug.txt log file, writing a one-time header block on first use.
- 0x449590 qsort_dword_array size=302 conf=? :: 
- 0x4496d0 qsort_dword_array_shortsort size=78 conf=? :: 
