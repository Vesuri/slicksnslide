# Source before PLAYERSR or TRACKS workflow. Check every held display with
# tools/check_menu_transition_hold.py RUN/.run/menu-transition-hold afterwards.
set $hold_count=0
set $hold_pending=0
break hold_title_menu_display
commands
  silent
  if platform->active
    if $hold_pending || shown_view != 0
      printf "MENU_HOLD_ENTRY_FAILED\n"
      quit 1
    end
    set $hold_pending=1
    set $hold_pixels=platform->views[0].bitmap->Planes[0]
    eval "dump binary memory .run/menu-transition-hold/%u-before.planar %p %p",$hold_count,$hold_pixels,$hold_pixels+64000
    eval "dump binary memory .run/menu-transition-hold/%u-before.palette %p %p",$hold_count,&view_palettes[0][0],&view_palettes[0][0]+768
  end
  continue
end
break slicks_amiga_platform_show
commands
  silent
  if $hold_pending
    if view==1 && $hold_pending==1
      set $hold_pending=2
      set $hold_pixels=platform->views[1].bitmap->Planes[0]
      eval "dump binary memory .run/menu-transition-hold/%u-held.planar %p %p",$hold_count,$hold_pixels,$hold_pixels+64000
      eval "dump binary memory .run/menu-transition-hold/%u-held.palette %p %p",$hold_count,&view_palettes[1][0],&view_palettes[1][0]+768
    else
      if view!=0 || $hold_pending!=2 || shown_view!=1
        printf "MENU_HOLD_PUBLICATION_FAILED\n"
        quit 1
      end
      eval "dump binary memory .run/menu-transition-hold/%u-after.planar %p %p",$hold_count,$hold_pixels,$hold_pixels+64000
      eval "dump binary memory .run/menu-transition-hold/%u-after.palette %p %p",$hold_count,&view_palettes[1][0],&view_palettes[1][0]+768
      set $hold_pending=0
      set $hold_count=$hold_count+1
    end
  end
  continue
end
break slicks_amiga_platform_set_view
commands
  silent
  if $hold_pending==2 && (view!=0 || shown_view!=1)
    printf "MENU_HOLD_PALETTE_CHANGED_WHILE_VISIBLE\n"
    quit 1
  end
  continue
end
break *slicks_amiga_platform_end
commands
  silent
  if $hold_pending
    printf "MENU_HOLD_DISPLAY_TEARDOWN\n"
    quit 1
  end
  continue
end
