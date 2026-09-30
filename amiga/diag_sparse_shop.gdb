# SLICKS_WEAPON_CASE=S (NATURALWS): drivers 1 and 3 human, 2 and 4 inactive.
# Right selects driver 3, Enter buys, Backspace sells, Q is ignored, Escape. Checks the safe
# active-player mapping (decision D2) when the shop closes.
break slicks_amiga_platform_begin_io
commands
  silent
  printf "SPARSE driver=%d transactions=%u refreshes=%u cash=%d,%d,%d items=%d,%d,%d other_cash=%d now=%d\n",g_slicks_sparse_shop_driver,g_slicks_sparse_shop_transactions,g_slicks_sparse_shop_refreshes,g_slicks_sparse_shop_cash[0],g_slicks_sparse_shop_cash[1],g_slicks_sparse_shop_cash[2],g_slicks_sparse_shop_item_count[0],g_slicks_sparse_shop_item_count[1],g_slicks_sparse_shop_item_count[2],g_slicks_sparse_shop_other_cash,g_slicks_setup_session.cash[0]
  if g_slicks_sparse_shop_driver==2 && g_slicks_sparse_shop_transactions==2 && g_slicks_sparse_shop_refreshes==3 && g_slicks_sparse_shop_cash[1]<g_slicks_sparse_shop_cash[0] && g_slicks_sparse_shop_item_count[1]==g_slicks_sparse_shop_item_count[0]+1 && g_slicks_sparse_shop_cash[2]>g_slicks_sparse_shop_cash[1] && g_slicks_sparse_shop_item_count[2]==g_slicks_sparse_shop_item_count[0] && g_slicks_setup_session.cash[0]==g_slicks_sparse_shop_other_cash
    printf "SPARSE_SHOP_OK\n"
    quit
  end
  printf "SPARSE_SHOP_FAILED\n"
  quit 1
end
break slicks_diag_shop_ready
commands
  silent
  printf "SPARSE_SHOP_READY driver=%d transactions=%u\n",g_slicks_sparse_shop_driver,g_slicks_sparse_shop_transactions
  continue
end
break slicks_diag_race_load_failed
commands
  silent
  printf "SPARSE_SHOP_FAILED race load\n"
  quit 1
end
continue
