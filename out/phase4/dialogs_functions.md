# dialogs module: 3 functions (address, current Ghidra name, size, agent confidence, summary)

- 0x57e1f0 dialog_box_show_localized size=165 conf=0.7 :: Shows a modal dialog box by resource name, trying the current UI language, then falling back to English, then to a default resource lookup.
- 0x57e350 dialog_static_hyperlink_subclass_proc size=363 conf=0.6 :: Window-subclass procedure implementing hover/hand-cursor tracking and cleanup for a hyperlink-style static control installed by FUN_0057e4c0.
- 0x57e4c0 dialog_static_hyperlink_install size=209 conf=0.65 :: Converts a static text control into a clickable, underlined hyperlink-style control by subclassing it and its parent and applying an underlined font.
