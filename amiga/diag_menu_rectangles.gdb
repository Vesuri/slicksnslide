# Source before a menu workflow fixture. Capture every shared publication;
# verify with tools/check_menu_publications.py DIRECTORY afterwards.
# Create .run/menu-rectangles before launching and use a fresh directory.
set $menu_publications = 0
break *slicks_amiga_player_menu_clear_dirty
commands
  silent
  set $menu = *(struct SlicksAmigaPlayerMenu **)($sp+4)
  set $bitmap = g_slicks_diag_profile_platform->views[0].bitmap
  if $bitmap->Depth != 8 || $bitmap->BytesPerRow != 320
    quit 1
  end
  eval "dump binary memory .run/menu-rectangles/%u.chunky %p %p", $menu_publications, $menu->renderer.ui.pixels, $menu->renderer.ui.pixels+64000
  eval "dump binary memory .run/menu-rectangles/%u.planar %p %p", $menu_publications, $bitmap->Planes[0], $bitmap->Planes[0]+64000
  printf "MENU_RECTANGLE_PUBLICATION %u rectangles=%u\n", $menu_publications, $menu->dirty_count
  set $rect = 0
  while $rect < $menu->dirty_count
    printf "MENU_RECTANGLE %u %u %u %u\n", $menu->dirty[$rect].left, $menu->dirty[$rect].top, $menu->dirty[$rect].right, $menu->dirty[$rect].bottom
    set $rect = $rect+1
  end
  set $menu_publications = $menu_publications+1
  continue
end
