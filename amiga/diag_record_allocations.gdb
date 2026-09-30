# OPTIONSBW fails each of the five record-read/view storage acquisitions once.
# Ordinary Retry input must eventually show/save the table without reinsertion.
set $expect_record_allocations=1
source diag_standings.gdb
