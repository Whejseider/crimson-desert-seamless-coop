// Co-op+ research (2026-11-07): pre-script for the full analysis. The first full run (222 min) died of
// java.lang.OutOfMemoryError (Address Table analyzer, a 20 GB heap) and saved nothing: the exe's `.tls$` section
// (RVA 0x7A44000, 0xBDE1F34 = 190 MB, read/write/execute) is the protector's code -- obfuscated, jumps into
// non-existing memory, millions of junk instruction runs -- and the analyzers' address sets over it blow up.
// The game's own code is `.text1` (85 MB). This removes `.tls$` from the program before analysis; a call from the
// game into it (the few relocated/virtualised functions, e.g. CheckAIBattleType's jmp to 0xD06B630) then shows as a
// call into unmapped memory, which is what it decompiled to anyway. Nothing else is changed.
// @category CoopPlus
import ghidra.app.script.GhidraScript;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.mem.MemoryBlock;

public class StripProtected extends GhidraScript {
    @Override
    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        for (MemoryBlock b : mem.getBlocks()) {
            println("block " + b.getName() + " " + b.getStart() + " size 0x" + Long.toHexString(b.getSize())
                + (b.isExecute() ? " x" : "") + (b.isWrite() ? " w" : ""));
        }
        for (MemoryBlock b : mem.getBlocks()) {
            if (b.getName().equals(".tls$")) {
                println("removing " + b.getName() + " (" + (b.getSize() >> 20) + " MB, the protector's code)");
                mem.removeBlock(b, monitor);
            }
        }
    }
}
