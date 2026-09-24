set $saved = 0
break slicks_diag_setup_saved
commands
  silent
  set $p = &g_slicks_setup_session.players
  printf "SHARED_SAVE result=%u count=%u profiles=%u selected=%d/%d/%d/%d roles=%d/%d/%d/%d\n", g_slicks_setup_save_report.result, $p->count, g_slicks_profiles.count, $p->selected[0],$p->selected[1],$p->selected[2],$p->selected[3],$p->participation[0],$p->participation[1],$p->participation[2],$p->participation[3]
  if g_slicks_setup_save_report.result || g_slicks_profiles.count != 3 || $p->count != 3
    quit 1
  end
  set $rank = 0
  set $i = 0
  while $i < 4
    if $i == 2
      if $p->selected[$i] || $p->participation[$i]
        quit 1
      end
    else
      if $p->selected[$i] != 2 || $p->participation[$i] != -1 || $p->order[$rank] != $i
        printf "SHARED_ROLE_FAILED slot=%u rank=%u order=%u\n",$i,$rank,$p->order[$rank]
        quit 1
      end
      set $j = 0
      while $j < 6
        if $p->colours[$i][$j] != slicks_original_fallback_colours[$rank][$j]
          printf "SHARED_COLOUR_FAILED slot=%u component=%u actual=%u expected=%u\n",$i,$j,$p->colours[$i][$j],slicks_original_fallback_colours[$rank][$j]
          quit 1
        end
        set $j = $j+1
      end
      set $rank = $rank+1
    end
    set $i = $i+1
  end
  dump binary memory .run/setup-shared-human-v1/saved-selection.bin &$p->selected &$p->selected+1
  dump binary memory .run/setup-shared-human-v1/saved-colours.bin &$p->colours &$p->colours+1
  set $saved = 1
  printf "SHARED_HUMAN_NATIVE_SAVED_THREE_DISTINCT_COLOURS\n"
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$saved || g_slicks_diag_restore_status != 0x1f
    quit 1
  end
  printf "SHARED_HUMAN_SAVE_RESTORED\n"
  quit
end
continue
