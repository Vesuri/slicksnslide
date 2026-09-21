// Disassemble live block seeds and name a small curated function set.
// Args: live-entry CSV, image load base, curated-symbol CSV.
//@category Slicks
import java.io.BufferedReader;
import java.io.FileReader;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;

public class SeedLiveMap extends GhidraScript {
    private long parseHex(String value) {
        return Long.parseLong(value.trim().replaceFirst("^(0x|\\$)", ""), 16);
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 3) {
            throw new IllegalArgumentException(
                "SeedLiveMap needs live-entry CSV, load base, and symbol CSV");
        }
        long loadBase = parseHex(args[1]);
        int blocks = 0;
        try (BufferedReader reader = new BufferedReader(new FileReader(args[0]))) {
            String line;
            while ((line = reader.readLine()) != null) {
                monitor.checkCancelled();
                line = line.trim();
                if (line.isEmpty() || line.startsWith("offset,")) continue;
                String[] fields = line.split(",", 2);
                Address address = toAddr(loadBase + parseHex(fields[0]));
                disassemble(address);
                blocks++;
            }
        }

        int functions = 0;
        try (BufferedReader reader = new BufferedReader(new FileReader(args[2]))) {
            String line;
            while ((line = reader.readLine()) != null) {
                monitor.checkCancelled();
                line = line.trim();
                if (line.isEmpty() || line.startsWith("offset,")) continue;
                String[] fields = line.split(",", 3);
                if (fields.length < 2) {
                    throw new IllegalArgumentException("bad symbol row: " + line);
                }
                Address address = toAddr(loadBase + parseHex(fields[0]));
                String name = fields[1].trim();
                disassemble(address);
                Function function = getFunctionAt(address);
                if (function == null) {
                    function = createFunction(address, name);
                } else {
                    function.setName(name, SourceType.USER_DEFINED);
                }
                if (fields.length == 3 && !fields[2].trim().isEmpty()) {
                    currentProgram.getListing().setComment(
                        address, CodeUnit.PLATE_COMMENT, fields[2].trim());
                }
                functions++;
            }
        }
        println("SeedLiveMap: disassembled " + blocks + " block seeds and named "
                + functions + " curated functions");
    }
}
