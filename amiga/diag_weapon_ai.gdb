# NATURALA: computers buy naturally on successive tracks. Human never fires.
set $starts=0
set $shots=0
break enter_prepared_race
commands
  silent
  set $starts=$starts+1
  set $r=race
  printf "AI_WEAPON_RACE %u selected=%d/%d/%d inventories:",$starts,$r->selected_weapon[1],$r->selected_weapon[2],$r->selected_weapon[3]
  set $d=1
  while $d<4
    set $w=5
    while $w<13
      printf " %d",$r->weapon_inventory[$d][$w]
      set $w=$w+1
    end
    set $d=$d+1
  end
  printf "\n"
  if !$r->weapons_enabled || $r->participation[0]>=0 || $r->participation[1]<=0 || $r->participation[2]<=0 || $r->participation[3]<=0
    quit 1
  end
  continue
end
break slicks_diag_pause_live_ready
commands
  silent
  printf "AI_WEAPON_RESULT frame=%u shots=%lu hits=%lu error=%u\n",$r->frame_count,$r->weapons.shots,$r->weapons.hits,g_slicks_diag_race_error
  if $r->weapon_inventory[0][5]!=6 || g_slicks_diag_race_error
    quit 1
  end
  set $shots=$shots+$r->weapons.shots
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$shots || g_slicks_diag_race_error || g_slicks_diag_restore_status!=0x1f
    printf "AI_WEAPON_FAILED starts=%u shots=%u error=%u restore=%u\n",$starts,$shots,g_slicks_diag_race_error,g_slicks_diag_restore_status
    quit 1
  end
  printf "NATIVE_SHOP_AI_TARGET_FIRE_OK races=%u shots=%u\n",$starts,$shots
  quit
end
continue
