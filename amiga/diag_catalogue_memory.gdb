# CATRECOV enables free-memory probes. Successful preparation sends no retry keys.
# Supersedes the old expected-OOM gate for the ordinary 10001-file fixture.
set $capture_race_memory=1
source diag_catalogue_limit.gdb
