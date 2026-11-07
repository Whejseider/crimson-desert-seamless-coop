// Co-op+ research (2026-11-07): RTTI for the game exe after the sliced analysis.
// Why: Ghidra's "Windows x86 PE RTTI Analyzer" only searches memory blocks whose names start with ".rdata" / ".data"
// / ".text". This exe names them ".rodata" / ".sdata" (code ".text1" is fine), so the analyzer found nothing
// (0 vftables, while the exe holds 15,171 ".?AV" RTTI type names, all in .rodata). Also
// RecoverClassesFromRTTIScript refuses a program not marked "analyzed" (the sliced runs never set that flag).
// This script: renames .rodata -> .rdata and .sdata -> .data (names only), runs the RTTI analyzer once over the
// whole program, marks the program analyzed, saves.
// @category CoopPlus
import ghidra.app.plugin.prototype.MicrosoftCodeAnalyzerPlugin.RttiAnalyzer;
import ghidra.app.script.GhidraScript;
import ghidra.app.util.importer.MessageLog;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.SymbolIterator;
import ghidra.program.util.GhidraProgramUtilities;

public class RttiFix extends GhidraScript {
    @Override
    public void run() throws Exception {
        end(true); // our own transactions below (see AnalyzeSliced.java)

        // The roles, from the bytes (2026-11-07): the TypeDescriptors (RTTI0, writable) are in ".rodata" = the real
        // .data; they point to type_info's vftable at RVA 0x58D9BB8 in ".arch" = the real .rdata. The analyzer looks
        // for RTTI0 in ".data*" blocks and for RTTI4 / vftables in ".rdata*", ".data*", ".text*". Start state either
        // the original names or the first try of this script (.rodata -> .rdata, .sdata -> .data).
        int t = currentProgram.startTransaction("rename data blocks");
        MemoryBlock rodata = null, sdata = null, arch = null;
        for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
            String n = b.getName();
            long rva = b.getStart().getOffset() - currentProgram.getImageBase().getOffset();
            if (rva == 0x6CD2000L) rodata = b;   // .rodata 0x879000 bytes, writable
            if (rva == 0x754B000L) sdata = b;    // .sdata 0x2D9000
            if (rva == 0x5597000L) arch = b;     // .arch 0x173B000
        }
        if (sdata != null) { println("block " + sdata.getName() + " -> .data_s"); sdata.setName(".data_s"); }
        if (rodata != null) { println("block " + rodata.getName() + " -> .data"); rodata.setName(".data"); }
        if (arch != null) { println("block " + arch.getName() + " -> .rdata"); arch.setName(".rdata"); }
        // The analyzer skips itself if "RTTI Found" was ever recorded (the earlier runs recorded false).
        currentProgram.getOptions("Program Information").removeOption("RTTI Found");
        currentProgram.endTransaction(t, true);
        currentProgram.save("rename data blocks", monitor);

        long t0 = System.currentTimeMillis();
        t = currentProgram.startTransaction("RTTI");
        boolean ok = false;
        try {
            MessageLog log = new MessageLog();
            RttiAnalyzer a = new RttiAnalyzer();
            ok = a.added(currentProgram, currentProgram.getMemory().getLoadedAndInitializedAddressSet(), monitor, log);
            if (log.hasMessages()) println("RTTI log: " + log.toString().substring(0, Math.min(4000, log.toString().length())));
        } finally {
            currentProgram.endTransaction(t, true);
        }
        int vf = 0;
        SymbolIterator si = currentProgram.getSymbolTable().getSymbols("vftable");
        while (si.hasNext()) { si.next(); vf++; }
        println("RTTI analyzer returned " + ok + " in " + (System.currentTimeMillis() - t0) / 1000 + " s; vftable symbols: " + vf);

        t = currentProgram.startTransaction("mark analyzed");
        GhidraProgramUtilities.markProgramAnalyzed(currentProgram);
        currentProgram.endTransaction(t, true);
        currentProgram.save("RTTI", monitor);
        currentProgram.clearUndo();
        start();
    }
}
