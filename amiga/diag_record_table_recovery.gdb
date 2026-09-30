# OPTIONSBT retries a partial records-surface allocation; OPTIONSBU skips
# the table only. Both must calculate and persist earned records normally.
set $expect_record_table=1
source diag_standings.gdb
