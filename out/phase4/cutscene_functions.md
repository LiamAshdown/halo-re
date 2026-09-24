# cutscene module: 15 functions (address, current Ghidra name, size, agent confidence, summary)

- 0x449590 qsort_dword_array size=302 conf=0.5 :: Sorts an array of 4-byte (dword/pointer) elements in place using quicksort with an insertion-sort fallback for small partitions and a caller-supplied comparison callback.
- 0x4496d0 qsort_dword_array_shortsort size=78 conf=0.5 :: Sorts a small sub-array of 4-byte elements in place, used as the small-partition fallback for the quicksort routine.
- 0x449720 cutscene_start size=92 conf=0.5 :: Begins a cutscene: saves a default music gain if none is saved, forces music to full volume, and marks the letterbox/cutscene state as active.
- 0x449780 FUN_00449780 size=473 conf=0.4 :: Draws one letterbox bar using a screen rectangle supplied via ECX, called once each for the top and bottom bars.
- 0x449960 cutscene_title_queue size=95 conf=0.55 :: Queues a cutscene title for display in one of up to four concurrent slots, scheduling it to start fading after the given delay in seconds.
- 0x4499c0 chimera__letterbox size=1261 conf=0.5 :: Per-frame update that fades and draws the cutscene letterbox bars and renders any queued cutscene title/subtitle text with time-based fade in/out.
- 0x449eb0 cutscene_stop size=203 conf=0.55 :: Ends a cutscene: restores the previous music gain, clears cutscene/letterbox state, resets camera-related output fields, and surfaces any queued cutscene error.
- 0x449f80 FUN_00449f80 size=70 conf=0.35 :: Case-insensitively searches a name-indexed record array for a matching entry and returns its index, or -1 if not found.
- 0x449fd0 FUN_00449fd0 size=136 conf=0.3 :: Copies a sequence of variable-sized fields from a source buffer into a destination structure according to a per-version field-layout table, used for migrating older tag/data formats.
- 0x44a110 FUN_0044a110 size=64 conf=0.45 :: Applies a one-byte-per-axis compressed rotation delta to a wrapped 1000-unit fixed-point angle pair (small delta format).
- 0x44a150 FUN_0044a150 size=61 conf=0.45 :: Applies a two-byte-per-axis compressed rotation delta to a wrapped 1000-unit fixed-point angle pair (large delta format).
- 0x44a190 recorded_animation_angle_to_vector size=62 conf=0.5 :: Converts a compressed 1000-unit fixed-point yaw/pitch pair into a normalized 3D direction vector.
- 0x44a1d0 FUN_0044a1d0 size=439 conf=0.45 :: Decodes one small-delta-format compressed animation frame, updating the forward/left/up orientation vectors based on per-axis change flags.
- 0x44a390 FUN_0044a390 size=437 conf=0.45 :: Decodes one large-delta-format compressed animation frame, updating the forward/left/up orientation vectors based on per-axis change flags.
- 0x44a790 FUN_0044a790 size=133 conf=0.4 :: Applies an uncompressed absolute keyframe orientation to the forward/left/up vectors of an animation frame, sharing the direction across axes not explicitly excluded.
