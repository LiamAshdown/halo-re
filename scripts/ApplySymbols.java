// Applies the text source-of-truth to the Ghidra program.
//   symbols/functions.txt   0xADDR name kind confidence source...   (kind: func|code|data)
//   symbols/prototypes.txt  0xADDR <C prototype>;
//   types/*.h               C headers parsed into the program's data type manager
// Usage (headless): -postScript ApplySymbols.java <repo root>
//@category Halo
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.function.ApplyFunctionSignatureCmd;
import ghidra.app.util.cparser.C.CParser;
import ghidra.app.util.parser.FunctionSignatureParser;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class ApplySymbols extends GhidraScript {
    int created, renamed, labels, data, protos, failed;

    // Header parse order. Each header is parsed by its own CParser against the program's data
    // type manager, so every type a header uses (by value or through a typedef'd pointer) must
    // already be in the manager when that header is parsed. Alphabetical order does not give
    // that: ai.h needs datum_index / data_array (memory.h) and the real_* types (math.h), cache.h
    // needs BitmapData / SoundPermutation (tags.h), camera.h, hs.h, interface.h, objects.h,
    // rasterizer.h, render.h, scenario.h, shaders.h, sound.h and structures.h need tags.h types,
    // cutscene.h needs unit_control_data (units.h), effects.h needs bsp_leaf_reference
    // (objects.h), and input.h needs player_control_settings / controls_gamepad_record /
    // ui_input_event / input_guid (interface.h) and control_binding_descriptor (saved_games.h).
    // Dependency order (checked by concatenating the headers in this order through gcc
    // -fsyntax-only, 32- and 64-bit, with no errors; no cycles):
    //   tags memory math objects units ai bitmaps cache camera cseries cutscene devices dialogs
    //   effects game hs interface items main models networking physics projectiles rasterizer
    //   render saved_games input scenario shaders shell sound structures text
    // i.e. tags, memory, math, objects, units first, then the rest alphabetically with input.h
    // moved right after saved_games.h. The only type defined twice is datum_index (cache.h and
    // memory.h, identical typedef). Any header not in this list is parsed afterwards, in
    // alphabetical order.
    static final String[] HEADER_ORDER = {
        "tags.h", "memory.h", "math.h", "objects.h", "units.h",
        "ai.h", "bitmaps.h", "cache.h", "camera.h", "cseries.h", "cutscene.h", "devices.h",
        "dialogs.h", "effects.h", "game.h", "hs.h", "interface.h", "items.h", "main.h",
        "models.h", "networking.h", "physics.h", "projectiles.h", "rasterizer.h", "render.h",
        "saved_games.h", "input.h", "scenario.h", "shaders.h", "shell.h", "sound.h",
        "structures.h", "text.h",
    };

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Path root = Paths.get(args.length > 0 ? args[0] : ".");
        applyHeaders(root.resolve("types"));
        applySymbols(root.resolve("symbols").resolve("functions.txt"));
        applyPrototypes(root.resolve("symbols").resolve("prototypes.txt"));
        println(String.format("ApplySymbols: created=%d renamed=%d code-labels=%d data-labels=%d prototypes=%d failed=%d",
            created, renamed, labels, data, protos, failed));
    }

    void applyHeaders(Path dir) throws Exception {
        if (!Files.isDirectory(dir)) return;
        DataTypeManager dtm = currentProgram.getDataTypeManager();
        List<Path> found = new ArrayList<>();
        try (DirectoryStream<Path> ds = Files.newDirectoryStream(dir, "*.h")) { for (Path p : ds) found.add(p); }
        Collections.sort(found);
        // the fixed dependency order first, then whatever else is present, alphabetically
        List<Path> hs = new ArrayList<>();
        for (String name : HEADER_ORDER) {
            Path p = dir.resolve(name);
            if (Files.exists(p)) hs.add(p);
            else println("header in HEADER_ORDER not found: " + name);
        }
        for (Path p : found) {
            if (!Arrays.asList(HEADER_ORDER).contains(p.getFileName().toString())) hs.add(p);
        }
        for (Path h : hs) {
            try {
                CParser parser = new CParser(dtm, true, null);
                try (InputStream in = Files.newInputStream(h)) { parser.parse(in); }
                println("parsed header " + h.getFileName() + " (" + parser.getTypes().size() + " types)");
            } catch (Exception e) {
                failed++;
                println("HEADER FAILED " + h.getFileName() + ": " + e.getMessage());
            }
        }
    }

    void applySymbols(Path file) throws Exception {
        if (!Files.exists(file)) { println("no " + file); return; }
        FunctionManager fm = currentProgram.getFunctionManager();
        SymbolTable st = currentProgram.getSymbolTable();
        for (String line : Files.readAllLines(file, java.nio.charset.StandardCharsets.ISO_8859_1)) {
            line = line.trim();
            if (line.isEmpty() || line.startsWith("#")) continue;
            String[] t = line.split("\\s+");
            if (t.length < 3) continue;
            Address addr;
            try { addr = toAddr(Long.parseLong(t[0].replaceFirst("^0[xX]", ""), 16)); } catch (Exception e) { failed++; continue; }
            String name = t[1], kind = t[2];
            try {
                if (kind.equals("func")) {
                    Function f = fm.getFunctionAt(addr);
                    if (f == null) {
                        Function enclosing = fm.getFunctionContaining(addr);
                        f = createFunction(addr, name);
                        if (f == null) {
                            // no code flow known here yet: disassemble then retry
                            disassemble(addr);
                            f = createFunction(addr, name);
                        }
                        if (f == null && enclosing != null) {
                            // Ghidra refuses to split when the enclosing body reaches addr by a jump:
                            // drop the enclosing function, create ours, then recreate the enclosing one.
                            Address encEntry = enclosing.getEntryPoint();
                            String encName = enclosing.getName();
                            boolean encDefault = enclosing.getSymbol().getSource() == SourceType.DEFAULT;
                            removeFunction(enclosing);
                            f = createFunction(addr, name);
                            Function re = createFunction(encEntry, encDefault ? null : encName);
                            if (re == null) println("WARNING: could not recreate " + encName + " at " + encEntry);
                            enclosing = null;
                        }
                        if (f == null) { failed++; println("could not create function at " + addr + " " + name); continue; }
                        created++;
                        if (enclosing != null) println("split " + enclosing.getName() + " at " + addr + " -> " + name);
                    }
                    if (!f.getName().equals(name) && f.getSymbol().getSource() != SourceType.USER_DEFINED) {
                        f.setName(name, SourceType.IMPORTED); renamed++;
                    }
                } else if (kind.equals("data")) {
                    Symbol existing = st.getPrimarySymbol(addr);
                    if (existing == null || existing.getSource() == SourceType.DEFAULT || existing.getSource() == SourceType.ANALYSIS) {
                        st.createLabel(addr, name, SourceType.IMPORTED).setPrimary();
                    } else if (!existing.getName().equals(name)) {
                        st.createLabel(addr, name, SourceType.IMPORTED); // secondary label, keep user's primary
                    }
                    data++;
                } else { // code hint label inside a function: never primary over a function symbol
                    Symbol s = st.createLabel(addr, name, SourceType.IMPORTED);
                    if (fm.getFunctionAt(addr) != null) { /* leave function name primary */ }
                    labels++;
                }
            } catch (Exception e) { failed++; println("FAILED " + line + ": " + e.getMessage()); }
        }
    }

    void applyPrototypes(Path file) throws Exception {
        if (!Files.exists(file)) return;
        FunctionSignatureParser p = new FunctionSignatureParser(currentProgram.getDataTypeManager(), null);
        for (String line : Files.readAllLines(file, java.nio.charset.StandardCharsets.ISO_8859_1)) {
            line = line.trim();
            if (line.isEmpty() || line.startsWith("#")) continue;
            int sp = line.indexOf(' ');
            if (sp < 0) continue;
            try {
                Address addr = toAddr(Long.parseLong(line.substring(0, sp).replaceFirst("^0[xX]", ""), 16));
                Function f = getFunctionAt(addr);
                if (f == null) { failed++; println("no function for prototype at " + addr); continue; }
                String sig = line.substring(sp + 1).trim();
                if (sig.endsWith(";")) sig = sig.substring(0, sig.length() - 1);
                // Ghidra's signature parser does not take the convention inline; strip it and set it separately
                String conv = null;
                java.util.regex.Matcher cm = java.util.regex.Pattern.compile("\\b(__cdecl|__stdcall|__fastcall|__thiscall)\\b").matcher(sig);
                if (cm.find()) { conv = cm.group(1); sig = sig.replace(cm.group(1), " ").replaceAll("\\s+", " ").trim(); }
                FunctionDefinitionDataType def = p.parse(f.getSignature(), sig);
                ApplyFunctionSignatureCmd cmd = new ApplyFunctionSignatureCmd(addr, def, SourceType.IMPORTED);
                if (cmd.applyTo(currentProgram, monitor)) {
                    protos++;
                    if (conv != null) { try { f.setCallingConvention(conv); } catch (Exception ce) { println("cc not set " + conv + " at " + addr + ": " + ce.getMessage()); } }
                } else { failed++; println("prototype rejected: " + line); }
            } catch (Exception e) { failed++; println("PROTO FAILED " + line + ": " + e.getMessage()); }
        }
    }
}
