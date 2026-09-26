# No per-frame breakpoints: measure native preparation, not debugger latency.
break enter_prepared_race
commands
  silent
  if g_slicks_diag_race_error || !race->chunky_authoritative
    quit 1
  end
  printf "RELEASE_STARTUP ticks=%lu race_bytes=%lu\n",g_slicks_load_ticks[9]-g_slicks_load_ticks[0],sizeof(*race)
  printf "RELEASE_STARTUP_OK\n"
  quit
end
continue
quit 1
