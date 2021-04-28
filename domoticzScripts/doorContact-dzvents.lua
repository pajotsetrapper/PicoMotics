-- DzVents script to monitor a door contact that sends an open / close signal.
-- Requires to create a dummy device (door switch), which is updated when an open / close signal is received.


return {
    active = true, -- set to false to disable this script
    on = {
        devices = {
            'Voordeur (open)',
            'Voordeur (dicht)',
        },
    },
    -- in case of a timer event or security event, device == nil
    execute = function(domoticz, device)
        dateString = os.date('%Y-%m-%d %H:%M:%S')
        if device.name == 'Voordeur (open)' then
            domoticz.devices('Voordeur').open()
            -- https://www.domoticz.com/wiki/DzVents:_next_generation_Lua_scripting
            domoticz.notify(
                device.name .. ': ' .. dateString,
                device.name .. ': ' .. dateString,
                PRIORITY_NORMAL,
                SOUND_DEFAULT,
                nil,
                NSS_PUSHOVER,
                nil)
        elseif (device.name == 'Voordeur (dicht)') then
            domoticz.devices('Voordeur').close()
        end
    end
}
