# Diagnostic-only initial BIOS-clock fraction, before the measured window.
# Caller sets $hud_phase=0..3 and launches with SLICKS_HUD_PHASE matching.
# Verify native initialization; do not write target memory through GDB.
if $hud_phase<0 || $hud_phase>3
  printf "INVALID_HUD_PHASE\n"
  quit 1
end
tbreak *slicks_race_step
commands
  silent
  printf "HUD_PHASE=%u remainder=%lu ticks=%u\n",$hud_phase,status_clock.remainder,status_clock.ticks
  if status_clock.remainder!=819200*$hud_phase
    printf "HUD_PHASE_NOT_APPLIED\n"
    quit 1
  end
  continue
end
source diag_benchmark.gdb
