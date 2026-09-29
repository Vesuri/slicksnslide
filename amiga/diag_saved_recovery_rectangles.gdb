# CHAMPSAVF with an existing synthetic E2E.SSS.new recovery artifact.
# The transaction must refuse to overwrite it; no primary save is created.
set $expect_recovery = 1
source diag_championship_save_failure_rectangles.gdb
