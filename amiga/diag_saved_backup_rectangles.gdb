# CHAMPSAVF with an existing synthetic E2E.SSS.bak recovery artifact.
# Verify the artifact is byte-identical outside GDB after this gate completes.
set $expect_recovery = 1
source diag_championship_save_failure_rectangles.gdb
