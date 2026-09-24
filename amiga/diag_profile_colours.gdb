# Default startup, no PLR file. Capture the palette supplied to the live HUD.
break *slicks_race_set_status_palette
commands
  silent
  set $profile_palette = *(unsigned char **)($sp+8)
  dump binary memory .run/profile-colours-v1/palette.bin $profile_palette $profile_palette+768
  continue
end
source diag_setup_defaults.gdb
