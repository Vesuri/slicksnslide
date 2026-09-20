// Export an auditable text listing. Arg0 = output path.
//@category Slicks
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;

public class ExportListing extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String out = args.length > 0 ? args[0] : "listing.txt";
        Listing listing = currentProgram.getListing();
        PrintWriter writer = new PrintWriter(new BufferedWriter(new FileWriter(out)));
        FunctionIterator functions = listing.getFunctions(true);
        int functionCount = 0;
        StringBuilder summary = new StringBuilder();
        while (functions.hasNext()) {
            Function function = functions.next();
            summary.append(String.format("; FUNC %-28s %s - %s%n",
                function.getName(), function.getEntryPoint(), function.getBody().getMaxAddress()));
            functionCount++;
        }
        writer.printf("; %d functions, %d instructions defined%n",
                      functionCount, listing.getNumInstructions());
        writer.print(summary);
        writer.println(";----------------------------------------------------------");
        InstructionIterator instructions = listing.getInstructions(true);
        while (instructions.hasNext()) {
            Instruction instruction = instructions.next();
            Address address = instruction.getAddress();
            Symbol symbol = getSymbolAt(address);
            if (symbol != null && symbol.getSource() != SourceType.DEFAULT)
                writer.printf("%s:%n", symbol.getName());
            StringBuilder bytes = new StringBuilder();
            for (byte value : instruction.getBytes())
                bytes.append(String.format("%02X ", value));
            String comment = instruction.getComment(CodeUnit.EOL_COMMENT);
            writer.printf("%s  %-20s %-30s%s%n", address, bytes.toString().trim(),
                          instruction.toString(), comment == null ? "" : "  ; " + comment);
        }
        writer.close();
        println("wrote " + out + " (" + functionCount + " functions, "
                + listing.getNumInstructions() + " instructions)");
    }
}
