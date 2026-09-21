// Disassemble and promote explicit entry addresses before auto-analysis.
//@category Slicks
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class MarkFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        for (String argument : getScriptArgs()) {
            long offset = Long.parseUnsignedLong(
                argument.startsWith("0x") ? argument.substring(2) : argument,
                16);
            Address address = currentProgram.getAddressFactory()
                .getDefaultAddressSpace().getAddress(offset);
            disassemble(address);
            Function function = getFunctionAt(address);
            if (function == null)
                function = createFunction(address, null);
            if (function == null)
                throw new IllegalArgumentException(
                    "cannot create function at " + argument);
        }
    }
}
