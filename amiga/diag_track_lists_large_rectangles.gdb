source diag_menu_rectangles.gdb
break *free_picker
commands
  silent
  set $freed_picker = *(struct SlicksAmigaProfilePicker **)($sp+4)
  if $freed_picker
    printf "LIST_PICKER_RELEASE names=%p requested=%lu\n",$freed_picker->owned_names,$freed_picker->owned_names_size
  end
  continue
end
source diag_track_lists_large.gdb
