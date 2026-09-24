// Dumps per-function metadata for the whole program to <outdir>/functions.json:
// address, name, name source, size, thunk, lib (FID/thunk/external), callees, callers, strings, globals, signature.
// Usage (headless): -postScript ExportMeta.java <outdir>
//@category Halo
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.MemoryBlock;
import java.io.*;
import java.util.*;

public class ExportMeta extends GhidraScript {
    static String esc(String s) {
        StringBuilder b = new StringBuilder();
        for (char c : s.toCharArray()) {
            if (c == '"' || c == '\\') b.append('\\').append(c);
            else if (c < 0x20 || c > 0x7e) b.append(String.format("\\u%04x", (int) c));
            else b.append(c);
        }
        return b.toString();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = new File(args.length > 0 ? args[0] : ".");
        outDir.mkdirs();
        FunctionManager fm = currentProgram.getFunctionManager();
        ReferenceManager rm = currentProgram.getReferenceManager();
        Listing listing = currentProgram.getListing();
        BookmarkManager bm = currentProgram.getBookmarkManager();
        int total = fm.getFunctionCount(), n = 0;
        try (PrintWriter w = new PrintWriter(new FileWriter(new File(outDir, "functions.json")))) {
            w.println("[");
            boolean first = true;
            for (Function f : fm.getFunctions(true)) {
                if (monitor.isCancelled()) break;
                n++;
                if (n % 500 == 0) monitor.setMessage("meta " + n + "/" + total);
                Address entry = f.getEntryPoint();
                boolean fid = false;
                for (Bookmark b : bm.getBookmarks(entry)) {
                    if (b.getCategory().contains("Function ID") || b.getComment().contains("Library Function")) fid = true;
                }
                boolean lib = fid || f.isThunk() || f.isExternal();
                // callees
                Set<String> callees = new TreeSet<>();
                for (Function c : f.getCalledFunctions(monitor)) callees.add(c.getEntryPoint().toString() + ":" + c.getName());
                int callers = f.getCallingFunctions(monitor).size();
                // strings and globals referenced from the body
                Set<String> strings = new LinkedHashSet<>();
                Set<String> globals = new LinkedHashSet<>();
                InstructionIterator it = listing.getInstructions(f.getBody(), true);
                while (it.hasNext()) {
                    Instruction ins = it.next();
                    for (Reference r : ins.getReferencesFrom()) {
                        if (!r.isMemoryReference() || r.getReferenceType().isFlow()) continue;
                        Address to = r.getToAddress();
                        Data d = listing.getDataContaining(to);
                        if (d != null && d.hasStringValue()) {
                            Object v = d.getValue();
                            if (v != null && strings.size() < 40) strings.add(v.toString());
                        } else {
                            MemoryBlock blk = currentProgram.getMemory().getBlock(to);
                            if (blk != null && !blk.isExecute() && globals.size() < 60) {
                                Symbol s = currentProgram.getSymbolTable().getPrimarySymbol(to);
                                globals.add(to.toString() + ":" + (s != null ? s.getName() : "?"));
                            }
                        }
                    }
                }
                StringBuilder sb = new StringBuilder();
                sb.append(first ? "" : ",\n").append("{");
                sb.append("\"addr\":\"").append(entry).append("\",");
                sb.append("\"name\":\"").append(esc(f.getName())).append("\",");
                sb.append("\"name_source\":\"").append(f.getSymbol().getSource()).append("\",");
                sb.append("\"size\":").append(f.getBody().getNumAddresses()).append(",");
                sb.append("\"thunk\":").append(f.isThunk()).append(",");
                sb.append("\"fid\":").append(fid).append(",");
                sb.append("\"lib\":").append(lib).append(",");
                sb.append("\"cc\":\"").append(esc(f.getCallingConventionName())).append("\",");
                sb.append("\"signature\":\"").append(esc(f.getSignature().getPrototypeString())).append("\",");
                sb.append("\"callers\":").append(callers).append(",");
                sb.append("\"callees\":[");
                boolean c1 = true; for (String c : callees) { sb.append(c1 ? "" : ",").append('"').append(esc(c)).append('"'); c1 = false; }
                sb.append("],\"strings\":[");
                c1 = true; for (String s : strings) { sb.append(c1 ? "" : ",").append('"').append(esc(s)).append('"'); c1 = false; }
                sb.append("],\"globals\":[");
                c1 = true; for (String g : globals) { sb.append(c1 ? "" : ",").append('"').append(esc(g)).append('"'); c1 = false; }
                sb.append("]}");
                w.print(sb);
                first = false;
            }
            w.println("\n]");
        }
        println("ExportMeta: wrote " + n + " functions to " + outDir);
    }
}
