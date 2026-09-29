# Use REGCHECKT and an explicitly supplied private registration key.
# Do not print or dump the owner's name or key material.
set $owner_rectangles=0
break *publish_title_dirty
commands
  silent
  set $r=0
  while $r<title_dirty.count
    if title_dirty.rects[$r].top==190
      if title_dirty.rects[$r].left<=0 || title_dirty.rects[$r].right>320 || title_dirty.rects[$r].bottom>200
        printf "TITLE_OWNER_BOUNDS_FAILED\n"
        quit 1
      end
      set $owner_rectangles=$owner_rectangles+1
    end
    set $r=$r+1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  if !$owner_rectangles || g_slicks_title_seen_modes!=31 || g_slicks_title_seen_roles!=7 || g_slicks_title_seen_counts!=3 || g_slicks_title_dirty_checks<15 || g_slicks_title_dirty_errors || g_slicks_diag_restore_status!=31
    printf "TITLE_OWNER_PUBLICATION_FAILED rectangles=%u checks=%lu errors=%lu restore=%u\n",$owner_rectangles,g_slicks_title_dirty_checks,g_slicks_title_dirty_errors,g_slicks_diag_restore_status
    quit 1
  end
  printf "TITLE_OWNER_BOUNDED_PUBLICATION_OK rectangles=%u checks=%lu\n",$owner_rectangles,g_slicks_title_dirty_checks
  quit
end
continue
quit 1
