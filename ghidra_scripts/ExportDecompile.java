// Export selected functions as decompiler C for reverse-engineering notes.
// Args: output path, then one or more function names.
//@category Slicks
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.PrintWriter;

public class ExportDecompile extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2)
            throw new IllegalArgumentException(
                "usage: ExportDecompile output function [function ...]");

        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        try (PrintWriter writer = new PrintWriter(
                 new BufferedWriter(new FileWriter(args[0])))) {
            for (int i = 1; i < args.length; ++i) {
                final String functionName = args[i];
                Function function = getGlobalFunctions(functionName).stream()
                    .findFirst().orElse(null);
                if (function == null && functionName.startsWith("0x")) {
                    Address address = currentProgram.getAddressFactory()
                        .getDefaultAddressSpace()
                        .getAddress(Long.parseUnsignedLong(
                            functionName.substring(2), 16));
                    function = getFunctionAt(address);
                    if (function == null) {
                        disassemble(address);
                        function = createFunction(address, null);
                    }
                }
                if (function == null)
                    throw new IllegalArgumentException(
                        "function not found: " + functionName);
                DecompileResults result = decompiler.decompileFunction(
                    function, 120, monitor);
                if (!result.decompileCompleted())
                    throw new IllegalStateException(
                        args[i] + ": " + result.getErrorMessage());
                writer.printf("/* %s at %s */%n", function.getName(),
                              function.getEntryPoint());
                writer.println(result.getDecompiledFunction().getC());
            }
        } finally {
            decompiler.dispose();
        }
        println("wrote " + args[0]);
    }
}
