-- Emulator-side input only: never write guest memory or custom registers.
-- GDB creates a host handshake after the native race begins polling devices.
local handshake = assert(os.getenv("SLICKS_JOYSTICK_HANDSHAKE"))
local tick, throttle = 0, nil
function on_uae_vsync()
    if not throttle then
        local f = io.open(handshake, "rb")
        if not f then return end
        local value = f:read("*a")
        f:close()
        if #value == 0 then return end
        local axis = false
        for i = 1, #value do if value:byte(i) ~= 0 then axis = true end end
        throttle = axis and "JOY2_UP" or "JOY2_FIRE_BUTTON"
    end
    tick = tick + 1
    if tick == 30 then
        uae_write_config(throttle .. " 1")
        uae_log("SLICKS_JOYSTICK_THROTTLE\n")
    elseif tick == 100 then
        uae_write_config("JOY2_RIGHT 1")
        uae_log("SLICKS_JOYSTICK_RIGHT\n")
    elseif tick == 170 then
        uae_write_config("JOY2_RIGHT 0 " .. throttle .. " 0")
        uae_log("SLICKS_JOYSTICK_RELEASE\n")
    end
end
