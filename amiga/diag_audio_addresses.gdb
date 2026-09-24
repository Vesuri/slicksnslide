# Instruction offsets are for tmp/SlicksAudioNoDMA. Fail on a changed binary.
if *(unsigned short *)((char *)start_looping_channel+0x64) != 0x2092 || *(unsigned short *)((char *)start_looping_channel+0x78) != 0x3189 || *(unsigned short *)((char *)slicks_amiga_audio_update+0x36) != 0x26e8
  echo AUDIO_TRACE_BINARY_MISMATCH\n
  quit 1
end
set logging file .run/audio-addresses/writes.log
set logging overwrite on
set logging redirect on
set logging enabled on
set $writes = 0
define audio_write
  set $addr = (unsigned long)$arg0
  set $size = (unsigned long)$arg1
  set $value = (unsigned long)$arg2
  set $valid = 0
  if $addr == 0xdff096
    set $valid = $size == 2 && ($value & 0x7ff0) == 0
  else
    if $addr >= 0xdff0a0 && $addr <= 0xdff0d8
      set $off = ($addr - 0xdff0a0) & 15
      set $valid = ($size == 4 && $off == 0) || ($size == 2 && ($off == 4 || $off == 6 || $off == 8))
    end
  end
  printf "AUDIO_WRITE FRAME=%u PC=%08x ADDRESS=%08x SIZE=%u VALUE=%08x VALID=%u\n", g_slicks_diag_race_frame, $pc, $addr, $size, $value, $valid
  set $writes = $writes + 1
  if !$valid
    echo AUDIO_TRACE_INVALID_ADDRESS_OR_MASK\n
    set logging redirect off
    quit 1
  end
end
break *((char *)start_looping_channel+0x2c)
commands
  silent
  audio_write 0xdff096 2 ($d2&65535)
  continue
end
break *((char *)start_looping_channel+0x64)
commands
  silent
  audio_write $a0 4 (*(unsigned*)$a2)
  continue
end
break *((char *)start_looping_channel+0x6e)
commands
  silent
  audio_write $a0 2 ($d1&65535)
  continue
end
break *((char *)start_looping_channel+0x72)
commands
  silent
  audio_write $a0 2 ((*(short*)($sp+10))&65535)
  continue
end
break *((char *)start_looping_channel+0x78)
commands
  silent
  audio_write ((unsigned)$a0+0xdff008) 2 ((unsigned)$a1&65535)
  continue
end
break *((char *)start_looping_channel+0x8e)
commands
  silent
  audio_write 0xdff096 2 ($d0&65535)
  continue
end
break *((char *)start_looping_channel+0xa0)
commands
  silent
  audio_write 0xdff096 2 ($d0&65535)
  continue
end
break *((char *)slicks_amiga_audio_update+0x36)
commands
  silent
  audio_write $a3 4 (*(unsigned*)($a0+216))
  continue
end
break *((char *)slicks_amiga_audio_update+0x3a)
commands
  silent
  audio_write $a3 2 1
  continue
end
break *((char *)slicks_amiga_audio_update+0x5e)
commands
  silent
  audio_write 0xdff096 2 2
  continue
end
break *((char *)slicks_amiga_audio_update+0x7e)
commands
  silent
  audio_write 0xdff096 2 4
  continue
end
break *((char *)slicks_amiga_audio_update+0xe8)
commands
  silent
  audio_write 0xdff0a6 2 ($d1&65535)
  continue
end
break *((char *)slicks_amiga_audio_update+0x116)
commands
  silent
  audio_write 0xdff0a6 2 ($d1&65535)
  continue
end
break slicks_diag_gameplay_ready
commands
  silent
  printf "AUDIO_TRACE_COMPLETE FRAME=%u WRITES=%u DMA=%x\n", g_slicks_diag_race_frame, $writes, (*(unsigned short *)0xdff002)&15
  set logging enabled off
  set logging redirect off
  printf "AUDIO_TRACE_COMPLETE FRAME=%u WRITES=%u\n", g_slicks_diag_race_frame, $writes
  disable
  continue
end
continue
