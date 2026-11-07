// Co-op+ research (2026-11-07): the auto-analysis in SLICES, each committed and SAVED on its own.
// Why: both one-transaction runs died of OutOfMemoryError (24 GB heap). The second (without the protector section)
// got through the whole analysis in 36 min and died in endTransaction: the change set of one transaction covering all
// 85 MB of code. Here the script commits the headless transaction it runs in, then analyses the code section in
// slices (4 MB), each in its own transaction, saves and drops the undo history after each, and records progress so a
// rerun goes on from the next slice.
//   args: <mode: base|deep> <progress file>
//   base = the FullOptions set (no Decompiler Parameter ID / aggressive finder); deep = every analyzer (DeepPass).
// Run with -noanalysis -postScript AnalyzeSliced.java base <file>. The marker "SLICED DONE" in the progress file says
// the whole pass finished; a slice whose analysis threw is logged "FAILED <exception>" there and skipped.
// @category CoopPlus
import ghidra.app.plugin.core.analysis.AutoAnalysisManager;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.mem.MemoryBlock;
import java.io.File;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class AnalyzeSliced extends GhidraScript {
    static final long SLICE = 4L << 20;

    void mem(String what) {
        Runtime r = Runtime.getRuntime();
        println(what + ": heap used " + ((r.totalMemory() - r.freeMemory()) >> 20) + " MB of " + (r.maxMemory() >> 20));
    }

    void setOpt(Map<String, String> opts, String k, String v) {
        if (opts.containsKey(k)) {
            setAnalysisOption(currentProgram, k, v);
            println("option " + k + " = " + v);
        }
    }

    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        String mode = a.length > 0 ? a[0] : "base";
        File progress = new File(a.length > 1 ? a[1] : "sliced-progress.txt");
        int done = -1;
        if (progress.exists()) {
            for (String line : Files.readAllLines(progress.toPath())) {
                if (line.startsWith(mode + " slice ")) done = Math.max(done, Integer.parseInt(line.split(" ")[2]));
            }
        }

        // Commit the transaction GhidraScript opened around run() (FlatProgramAPI.end); start() reopens one at the end
        // so GhidraScript's own end() finds it (the earlier probe trick made that end() throw "Transaction not found",
        // which hid the real exception of a failing slice).
        end(true);
        if (currentProgram.getCurrentTransactionInfo() != null) throw new IllegalStateException("still in a transaction");

        int tx = currentProgram.startTransaction("options " + mode);
        Map<String, String> opts = getCurrentAnalysisOptionsAndValues(currentProgram);
        if (mode.equals("deep")) {
            for (Map.Entry<String, String> e : opts.entrySet()) {
                String k = e.getKey();
                if (k.contains(".") || !"false".equals(e.getValue())) continue;
                if (k.startsWith("PDB") || k.contains("Condense Filler")) continue;
                setOpt(opts, k, "true");
            }
            setOpt(opts, "Decompiler Parameter ID.Commit Data Types", "true");
            setOpt(opts, "Decompiler Parameter ID.Commit Void Return Values", "true");
            setOpt(opts, "Decompiler Parameter ID.Analysis Decompiler Timeout (sec)", "120");
            setOpt(opts, "Decompiler Switch Analysis.Decompiler Timeout (sec)", "120");
            setOpt(opts, "Stack.Create Local Variables", "true");
            setOpt(opts, "Stack.Create Param Variables", "true");
            setOpt(opts, "Data Reference.Create Address Tables", "true");
            setOpt(opts, "Windows x86 PE RTTI Analyzer.Create RTTI Classes", "true");
            // Already ran in the base pass; in deep mode it re-ran without end (every signature change by Parameter
            // ID set it off again: script reload + a new decompiler process each time, 1 h on slice 0, 2026-11-07).
            setOpt(opts, "WindowsResourceReference", "false");
        } else {
            setOpt(opts, "Decompiler Parameter ID", "false");
            setOpt(opts, "Aggressive Instruction Finder", "false");
            setOpt(opts, "Condense Filler Bytes", "false");
            setOpt(opts, "Windows x86 PE RTTI Analyzer", "true");
            setOpt(opts, "Windows x86 PE Exception Handling", "true");
            setOpt(opts, "Demangler Microsoft", "true");
            setOpt(opts, "Function Start Search", "true");
            setOpt(opts, "ASCII Strings", "true");
            setOpt(opts, "Decompiler Switch Analysis", "true");
        }
        currentProgram.endTransaction(tx, true);

        AutoAnalysisManager mgr = AutoAnalysisManager.getAnalysisManager(currentProgram);
        mgr.initializeOptions();

        // Slices: the code section in 4 MB pieces first, then every other initialised block whole.
        List<AddressSet> slices = new ArrayList<>();
        List<String> names = new ArrayList<>();
        for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
            if (!b.isInitialized() || !b.isExecute()) continue;
            for (long off = 0; off < b.getSize(); off += SLICE) {
                Address s = b.getStart().add(off);
                Address e = b.getStart().add(Math.min(b.getSize(), off + SLICE) - 1);
                slices.add(new AddressSet(s, e));
                names.add(b.getName() + "+" + Long.toHexString(off));
            }
        }
        int firstData = slices.size();
        for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
            if (!b.isInitialized() || b.isExecute()) continue;
            if (b.getSize() <= SLICE) {
                slices.add(new AddressSet(b.getStart(), b.getEnd()));
                names.add(b.getName());
                continue;
            }
            for (long off = 0; off < b.getSize(); off += SLICE) {
                Address s = b.getStart().add(off);
                Address e = b.getStart().add(Math.min(b.getSize(), off + SLICE) - 1);
                slices.add(new AddressSet(s, e));
                names.add(b.getName() + "+" + Long.toHexString(off));
            }
        }
        println(mode + ": " + slices.size() + " slices, " + (done + 1) + " already done");

        boolean tablesOff = false;
        for (int i = done + 1; i < slices.size(); i++) {
            if (i >= firstData && !tablesOff) {
                // The data blocks: the Address Table analyzer ran out of the 24 GB heap on .arch (23 MB) in
                // removeNonSearchableMemory (the same analyzer killed run 1); it only looks for pointer tables in data.
                int ot = currentProgram.startTransaction("data options");
                setOpt(getCurrentAnalysisOptionsAndValues(currentProgram), "Create Address Tables", "false");
                currentProgram.endTransaction(ot, true);
                mgr.initializeOptions();
                tablesOff = true;
            }
            long t0 = System.currentTimeMillis();
            int t = currentProgram.startTransaction(mode + " slice " + i);
            boolean ok = false;
            String fail = "";
            try {
                mgr.reAnalyzeAll(slices.get(i));
                mgr.startAnalysis(monitor);
                ok = true;
            } catch (Throwable ex) {
                // A failing slice is logged (with its stack) and skipped; the pass goes on.
                java.io.StringWriter sw = new java.io.StringWriter();
                ex.printStackTrace(new java.io.PrintWriter(sw));
                printerr(mode + " slice " + i + " " + names.get(i) + " FAILED: " + sw);
                fail = " FAILED " + ex;
            } finally {
                currentProgram.endTransaction(t, ok);
            }
            currentProgram.save(mode + " slice " + i + " " + names.get(i), monitor);
            currentProgram.clearUndo();
            System.gc();
            Files.writeString(progress.toPath(), mode + " slice " + i + " " + names.get(i) + " "
                + (System.currentTimeMillis() - t0) / 1000 + " s\n",
                java.nio.file.StandardOpenOption.CREATE, java.nio.file.StandardOpenOption.APPEND);
            mem(mode + " slice " + i + "/" + slices.size() + " " + names.get(i) + " saved in "
                + (System.currentTimeMillis() - t0) / 1000 + " s, functions "
                + currentProgram.getFunctionManager().getFunctionCount());
        }
        Files.writeString(progress.toPath(), "SLICED DONE " + mode + "\n",
            java.nio.file.StandardOpenOption.CREATE, java.nio.file.StandardOpenOption.APPEND);
        println("SLICED DONE " + mode);
        start();
    }
}
