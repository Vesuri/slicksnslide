set $stage = 0
set $polls = 0
break poll_driver_devices
commands
  silent
  if !race->racing || !race->participation_ready || !driver_device_configuration || race->participation[2] >= 0 || driver_device_configuration->player_input[2] != 1
    printf "JOYSTICK_SETUP_FAILED\n"
    quit 1
  end
  if $stage == 0
    set g_slicks_diag_target_frame = 360000
    set $x = race->cars[2].x
    set $y = race->cars[2].y
    dump binary memory .run/joystick-v1/input-ready.bin &driver_device_configuration->field_062e (&driver_device_configuration->field_062e)+1
    set $stage = 1
    printf "JOYSTICK_READY axis_throttle=%u\n",driver_device_configuration->field_062e
  end
  set $polls = $polls+1
  set $controls = race->driver_controls[2] & 15
  if $stage == 1 && $controls == 1
    set $stage = 2
    printf "JOYSTICK_THROTTLE_REACHED_HUMAN_DRIVER\n"
  end
  if $stage == 2 && $controls == 9
    set $stage = 3
    printf "JOYSTICK_STEERING_REACHED_HUMAN_DRIVER\n"
  end
  if $stage == 3 && $controls == 0
    if (race->cars[2].x == $x && race->cars[2].y == $y) || g_slicks_diag_race_error
      printf "JOYSTICK_CAR_DID_NOT_MOVE\n"
      quit 1
    end
    printf "NATIVE_EMULATED_JOYSTICK_PRESS_STEER_RELEASE_MOVE_OK polls=%u delta=%ld,%ld\n",$polls,race->cars[2].x-$x,race->cars[2].y-$y
    quit
  end
  if $polls > 100000
    printf "JOYSTICK_INPUT_TIMEOUT stage=%u controls=%u\n",$stage,$controls
    quit 1
  end
  continue
end
break slicks_diag_system_restored
commands
  silent
  printf "JOYSTICK_EARLY_EXIT\n"
  quit 1
end
continue
