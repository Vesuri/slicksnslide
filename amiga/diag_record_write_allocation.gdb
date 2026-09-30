# OPTIONSBX retries the save-buffer allocation; OPTIONSBY skips that save.
set $expect_record_write_alloc=1
set $expected_record_read_errors=0
source diag_standings.gdb
