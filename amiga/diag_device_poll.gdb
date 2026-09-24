set $polls = 0
break poll_driver_devices
commands
  silent
  if !race->participation_ready || !driver_device_configuration || !race->racing || ticks == 0
    printf "DEVICE_POLL_STATE_FAILED\n"
    quit 1
  end
  set $polls = $polls+1
  if $polls == 10
    printf "NATIVE_RACE_DEVICE_POLL_LIFECYCLE_OK\n"
    quit
  end
  continue
end
continue
