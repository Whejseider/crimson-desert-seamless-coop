// Co-op+ research (2026-11-07): after the full analysis, a searchable index of the game exe in <dir>:
//   functions.tsv  rva, size, name (with its namespace, e.g. the RTTI class), callers, callees
//   calls.tsv      caller rva -> callee rva (direct calls)
//   classes.tsv    RTTI classes: class, vftable rva, slot count, slot rvas (first 64)
//   strings.tsv    string rva, the functions that use it, the text (one line, escaped)
// Addresses are RVAs (image base subtracted), as everywhere in the project's notes.
// @category CoopPlus
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.StringDataInstance;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;
import java.io.File;
import java.io.PrintWriter;
import java.util.TreeSet;

public class ExportIndex extends GhidraScript {
    long base;

    String rva(Address a) { return "0x" + Long.toHexString(a.getOffset() - base).toUpperCase(); }

    @Override
    public void run() throws Exception {
        String dir = getScriptArgs()[0];
        new File(dir).mkdirs();
        base = currentProgram.getImageBase().getOffset();
        Memory mem = currentProgram.getMemory();

        try (PrintWriter fo = new PrintWriter(new File(dir, "functions.tsv"), "UTF-8");
             PrintWriter co = new PrintWriter(new File(dir, "calls.tsv"), "UTF-8")) {
            fo.println("rva\tsize\tname\tcallers\tcallees");
            FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
            long n = 0;
            while (it.hasNext() && !monitor.isCancelled()) {
                Function f = it.next();
                TreeSet<Long> callees = new TreeSet<>();
                InstructionIterator ii = currentProgram.getListing().getInstructions(f.getBody(), true);
                while (ii.hasNext()) {
                    Instruction ins = ii.next();
                    if (!ins.getFlowType().isCall()) continue;
                    for (Reference r : ins.getReferencesFrom()) {
                        if (r.getReferenceType().isCall()) callees.add(r.getToAddress().getOffset());
                    }
                }
                int callers = currentProgram.getReferenceManager().getReferenceCountTo(f.getEntryPoint());
                fo.println(rva(f.getEntryPoint()) + "\t" + f.getBody().getNumAddresses() + "\t" + f.getName(true) + "\t" +
                           callers + "\t" + callees.size());
                for (long c : callees) co.println(rva(f.getEntryPoint()) + "\t0x" + Long.toHexString(c - base).toUpperCase());
                if (++n % 20000 == 0) println("functions: " + n);
            }
            println("functions: " + n);
        }

        try (PrintWriter out = new PrintWriter(new File(dir, "classes.tsv"), "UTF-8")) {
            out.println("class\tvftable\tslots\tslot_rvas");
            SymbolIterator si = currentProgram.getSymbolTable().getSymbols("vftable");
            long n = 0;
            while (si.hasNext()) {
                Symbol s = si.next();
                Address a = s.getAddress();
                StringBuilder slots = new StringBuilder();
                int count = 0;
                for (int i = 0; i < 512; i++) {
                    long p;
                    try { p = mem.getLong(a.add(8L * i)); } catch (Exception e) { break; }
                    Address t = toAddr(p);
                    if (p < base || getFunctionAt(t) == null) break;
                    if (count < 64) slots.append(count == 0 ? "" : " ").append(rva(t));
                    count++;
                }
                out.println(s.getParentNamespace().getName(true) + "\t" + rva(a) + "\t" + count + "\t" + slots);
                n++;
            }
            println("vftables: " + n);
        }

        try (PrintWriter out = new PrintWriter(new File(dir, "strings.tsv"), "UTF-8")) {
            out.println("rva\tfunctions\ttext");
            DataIterator di = currentProgram.getListing().getDefinedData(true);
            long n = 0;
            while (di.hasNext() && !monitor.isCancelled()) {
                Data d = di.next();
                if (!d.hasStringValue()) continue;
                String text = StringDataInstance.getStringDataInstance(d).getStringValue();
                if (text == null || text.length() < 3) continue;
                TreeSet<String> users = new TreeSet<>();
                ReferenceIterator ri = currentProgram.getReferenceManager().getReferencesTo(d.getAddress());
                while (ri.hasNext() && users.size() < 16) {
                    Function f = getFunctionContaining(ri.next().getFromAddress());
                    if (f != null) users.add(rva(f.getEntryPoint()));
                }
                String one = text.replace("\\", "\\\\").replace("\t", "\\t").replace("\r", "\\r").replace("\n", "\\n");
                if (one.length() > 400) one = one.substring(0, 400) + "...";
                out.println(rva(d.getAddress()) + "\t" + String.join(",", users) + "\t" + one);
                n++;
            }
            println("strings: " + n);
        }
    }
}
