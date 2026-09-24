set $checked = 0
set $palette_checked = 0
break *slicks_race_set_status_palette
commands
  silent
  set $palette = *(unsigned char **)($sp+8)
  set $p = &g_slicks_setup_session.players
  set $i = 0
  while $i < 4
    if $p->participation[$i]
      set $shade = 0
      while $shade < 5
        set $c = 0
        while $c < 3
          set $first = (signed char)$p->colours[$i][$c]
          set $last = (signed char)$p->colours[$i][$c+3]
          set $expected = (unsigned char)(($first*(4-$shade))/4+($last*$shade)/4)
          if $palette[(1+$i*5+$shade)*3+$c] != $expected
            printf "SHARED_HUMAN_PALETTE_FAILED slot=%u shade=%u channel=%u\n", $i,$shade,$c
            quit 1
          end
          set $c = $c+1
        end
        set $shade = $shade+1
      end
    end
    set $i = $i+1
  end
  set $palette_checked = 1
  continue
end
break *slicks_race_start
commands
  silent
  set $race = *(struct SlicksRaceRuntime **)($sp+4)
  set $p = &g_slicks_setup_session.players
  if g_slicks_setup_load_report.result || !g_slicks_setup_load_report.configuration_present || !g_slicks_setup_load_report.profiles_present || $p->count != 3
    quit 1
  end
  set $rank = 0
  set $i = 0
  while $i < 4
    if $i == 2
      if $p->selected[$i] || $race->participation[$i]
        quit 1
      end
    else
      if $p->selected[$i] != 2 || $race->participation[$i] != -1 || $p->order[$rank] != $i || $race->cars[$i].vehicle != $p->vehicle[$i]
        quit 1
      end
      set $j = 0
      while $j < 6
        if $p->colours[$i][$j] != slicks_original_fallback_colours[$rank][$j]
          quit 1
        end
        set $j = $j+1
      end
      set $rank = $rank+1
    end
    set $i = $i+1
  end
  dump binary memory .run/setup-shared-human-v1/reloaded-selection.bin &$p->selected &$p->selected+1
  dump binary memory .run/setup-shared-human-v1/reloaded-colours.bin &$p->colours &$p->colours+1
  set $checked = 1
  continue
end
break slicks_diag_frame_ready
commands
  silent
  if g_slicks_diag_ingame
    if !$checked || !$palette_checked || g_slicks_diag_race_error
      quit 1
    end
    printf "SHARED_HUMAN_RELOAD_RACE_OK_ASSIGNMENTS_VEHICLES_DISTINCT_PALETTE\n"
    quit
  end
  continue
end
continue
