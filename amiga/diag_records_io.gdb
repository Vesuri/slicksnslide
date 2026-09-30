# OPTIONSBC: two native races, record-read Close failure/retry, then standings.
# Separate from the full standings assertions to respect the debugger's finite
# breakpoint table. Compare all io-*-before/after dumps after this gate passes.
set $capture_record_io = 1
source diag_records_resident.gdb
break slicks_diag_system_restored
commands
  silent
  if $record_returns != 2 || $record_owner || $record_io_open || $record_io_count < 3 || g_slicks_diag_restore_status != 0x1f
    printf "RECORD_DISPLAY_IO_FAILED returns=%u windows=%u\n",$record_returns,$record_io_count
    quit 1
  end
  printf "RECORD_DISPLAY_IO_OK returns=%u windows=%u\n",$record_returns,$record_io_count
  quit
end
continue
