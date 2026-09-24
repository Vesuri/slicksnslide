# Explicit renderer fixture, not proof of original gameplay weapon state.
# SLICKS_WEAPON_HUD=1 FSUAE_RUN=.run/weapon-hud-v5; GDB observes only.
set $icon_calls = 0
set $status_calls = 0
break *slicks_race_set_status_palette
commands
  silent
  set $hud_palette = *(unsigned char **)($sp+8)
  continue
end
break *slicks_race_start
commands
  silent
  set $hud_race = *(struct SlicksRaceRuntime **)($sp+4)
  printf "SLICKS_WEAPON_HUD_FIXTURE_ENABLED (renderer test only)\n"
  printf "FIXTURE RACE=%p ENABLED=%u SELECTED=%d,%d,%d,%d\n",$hud_race,$hud_race->weapons_enabled,$hud_race->selected_weapon[0],$hud_race->selected_weapon[1],$hud_race->selected_weapon[2],$hud_race->selected_weapon[3]
  continue
end
break *slicks_race_draw_status
commands
  silent
  set $status_calls = $status_calls + 1
  set $status_race = *(struct SlicksRaceRuntime **)($sp+4)
  if $status_calls == 1
    printf "STATUS RACE=%p ENABLED=%u SELECTED=%d,%d,%d,%d READY=%u\n",$status_race,$status_race->weapons_enabled,$status_race->selected_weapon[0],$status_race->selected_weapon[1],$status_race->selected_weapon[2],$status_race->selected_weapon[3],$status_race->hud_weapon_icons[0].ready
    if $status_race != $hud_race || !$status_race->weapons_enabled || $status_race->selected_weapon[0] != 0
      printf "SLICKS_WEAPON_HUD_FIXTURE_STATE_FAILED\n"
      quit 1
    end
    disable 3
  end
  continue
end
break *slicks_draw_chunky_icon
commands
  silent
  set $icon_calls = $icon_calls + 1
  continue
end
break slicks_diag_bitmap_audit_failed
commands
  silent
  printf "SLICKS_WEAPON_HUD_BITMAP_FAILED FRAME=%u X=%u Y=%u\n",g_slicks_diag_audit_frame,g_slicks_diag_audit_x,g_slicks_diag_audit_y
  quit 1
end
break slicks_diag_gameplay_ready
commands
  silent
  if g_slicks_diag_race_error || g_slicks_diag_race_frame != 700 || $icon_calls < 12
    printf "SLICKS_WEAPON_HUD_FAILED FRAME=%u ICON_CALLS=%u ERROR=%u\n",g_slicks_diag_race_frame,$icon_calls,g_slicks_diag_race_error
    quit 1
  end
  set $bitmap = g_slicks_diag_profile_platform->views[1].bitmap
  if $bitmap->BytesPerRow != 320 || $bitmap->Depth != 8
    printf "SLICKS_WEAPON_HUD_BITMAP_LAYOUT_FAILED\n"
    quit 1
  end
  dump binary memory .run/weapon-hud-v5/frame700.chunky $hud_race->chunky $hud_race->chunky+64000
  dump binary memory .run/weapon-hud-v5/frame700.planar $bitmap->Planes[0] $bitmap->Planes[0]+64000
  dump binary memory .run/weapon-hud-v5/palette.bin $hud_palette $hud_palette+768
  printf "SLICKS_WEAPON_HUD_OK FRAME=%u ICON_CALLS=%u ERROR=%u\n",g_slicks_diag_race_frame,$icon_calls,g_slicks_diag_race_error
  quit
end
continue
