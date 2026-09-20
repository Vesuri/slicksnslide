// Seed physical real-mode entry points listed in entrypoints.csv.
//@category Slicks
import java.io.BufferedReader;
import java.io.InputStreamReader;
import ghidra.app.script.GhidraScript;
import generic.jar.ResourceFile;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;

public class MarkEntries extends GhidraScript {
    @Override
    public void run() throws Exception {
        ResourceFile csv = new ResourceFile(getSourceFile().getParentFile(), "entrypoints.csv");
        int marked = 0;
        try (BufferedReader reader = new BufferedReader(new InputStreamReader(csv.getInputStream()))) {
            String line;
            while ((line = reader.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty() || line.startsWith("#") || line.startsWith("addr,")) continue;
                String[] fields = line.split(",", 3);
                if (fields.length < 2) {
                    printerr("bad entry row: " + line);
                    continue;
                }
                String hex = fields[0].trim().replaceFirst("^(0x|\\$)", "");
                Address address = toAddr(Long.parseLong(hex, 16));
                String name = fields[1].trim();
                addEntryPoint(address);
                disassemble(address);
                Function function = getFunctionAt(address);
                if (function == null) {
                    createFunction(address, name);
                } else if (function.getSymbol().getSource() == SourceType.ANALYSIS
                        || function.getSymbol().getSource() == SourceType.DEFAULT) {
                    function.setName(name, SourceType.USER_DEFINED);
                }
                marked++;
            }
        }
        println("MarkEntries: seeded " + marked + " entries");
    }
}
