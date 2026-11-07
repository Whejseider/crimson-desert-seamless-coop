// Co-op+ research (2026-11-07): decompiles EVERY function of the analysed game exe into <dir>/c/<shard>.c, one shard
// per 1 MB of address space (exe+0x2E00000..0x2EFFFFF -> 02E.c), each function headed
//   // ===== exe+0x<rva> <name> (<size> bytes)
// so a grep for "exe+0x2ED9000" or a name finds it. Runs <threads> decompilers in parallel (default 12); a function
// that takes over 60 s is skipped and listed in <dir>/c/_failed.txt. Re-running skips shards that already exist.
// @category CoopPlus
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileOptions;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.util.task.TaskMonitor;
import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

public class ExportAllC extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File dir = new File(args[0], "c");
        dir.mkdirs();
        int threads = args.length > 1 ? Integer.parseInt(args[1]) : 12;
        long base = currentProgram.getImageBase().getOffset();

        TreeMap<Long, List<Function>> shards = new TreeMap<>();
        FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
        long total = 0;
        while (it.hasNext()) {
            Function f = it.next();
            if (f.isThunk() || f.isExternal()) continue;
            long shard = (f.getEntryPoint().getOffset() - base) >> 20;
            shards.computeIfAbsent(shard, k -> new ArrayList<>()).add(f);
            total++;
        }
        final long totalF = total;
        println("functions to decompile: " + total + " in " + shards.size() + " shards, " + threads + " threads");

        ThreadLocal<DecompInterface> tl = ThreadLocal.withInitial(() -> {
            DecompInterface d = new DecompInterface();
            DecompileOptions o = new DecompileOptions();
            o.grabFromProgram(currentProgram);
            d.setOptions(o);
            d.toggleCCode(true);
            d.toggleSyntaxTree(false);
            d.openProgram(currentProgram);
            return d;
        });
        AtomicLong done = new AtomicLong(), failed = new AtomicLong();
        PrintWriter fails = new PrintWriter(new FileWriter(new File(dir, "_failed.txt"), true));
        long t0 = System.currentTimeMillis();
        ExecutorService pool = Executors.newFixedThreadPool(threads);
        for (Map.Entry<Long, List<Function>> e : shards.entrySet()) {
            File out = new File(dir, String.format("%03X.c", e.getKey()));
            if (out.exists()) {
                done.addAndGet(e.getValue().size());
                continue;
            }
            List<Function> fs = e.getValue();
            pool.submit(() -> {
                StringBuilder sb = new StringBuilder(fs.size() * 2048);
                for (Function f : fs) {
                    long rva = f.getEntryPoint().getOffset() - base;
                    sb.append("// ===== exe+0x").append(Long.toHexString(rva).toUpperCase()).append(' ')
                      .append(f.getName(true)).append(" (").append(f.getBody().getNumAddresses()).append(" bytes)\n");
                    try {
                        DecompileResults r = tl.get().decompileFunction(f, 60, TaskMonitor.DUMMY);
                        if (r != null && r.decompileCompleted()) {
                            sb.append(r.getDecompiledFunction().getC());
                        } else {
                            sb.append("// decompile failed: ").append(r == null ? "?" : r.getErrorMessage()).append('\n');
                            failed.incrementAndGet();
                            synchronized (fails) { fails.println("0x" + Long.toHexString(rva).toUpperCase() + "\t" + (r == null ? "?" : r.getErrorMessage())); }
                        }
                    } catch (Exception ex) {
                        sb.append("// decompile error: ").append(ex).append('\n');
                        failed.incrementAndGet();
                    }
                    sb.append('\n');
                    long d = done.incrementAndGet();
                    if (d % 5000 == 0) {
                        long s = (System.currentTimeMillis() - t0) / 1000;
                        System.out.println("decompiled " + d + " / " + totalF + " (" + failed.get() + " failed), " + s + " s");
                    }
                }
                File tmp = new File(dir, out.getName() + ".tmp");
                try (PrintWriter w = new PrintWriter(tmp, "UTF-8")) { w.print(sb); } catch (Exception ex) { return; }
                tmp.renameTo(out);
            });
        }
        pool.shutdown();
        while (!pool.awaitTermination(30, TimeUnit.SECONDS)) {
            if (monitor.isCancelled()) { pool.shutdownNow(); break; }
        }
        fails.close();
        println("decompiled " + done.get() + " functions, " + failed.get() + " failed, " +
                (System.currentTimeMillis() - t0) / 1000 + " s");
    }
}
