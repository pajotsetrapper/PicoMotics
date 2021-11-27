return {
    
	on = {
		timer = {
			-- 'every minute',	-- uses sunset/sunrise info from Domoticz
			'10 minutes before sunset', --zondondergang
			'every 20 minutes between sunset and 22:00', --zonsondergang tot slapen
			'every 120 minutes between 22:00 and sunrise', --tijdens de nacht
			'every 15 minutes between  07:00 and sunrise', -- de ochtend
			
			'every 35 minutes between sunset and 22:00', --simulatie kelder/toilet/berging
			'every 15 minutes between sunrise and sunset',
		}
		
		security = {
		    domoticz.SECURITY_ARMEDAWAY,
		    domoticz.SECURITY_ARMEDHOME,
		    domoticz.SECURITY_DISARMED
		}
	},
	
	logging = {
		level = domoticz.LOG_INFO,
		marker = 'Aanwezigheidssimulatie',
	},
	
	execute = function(domoticz, item)
		-- domoticz.log('Event was triggered by ' .. timer.trigger, domoticz.LOG_INFO)
		
		if item.isSecurity
		    -- code to handle case where system being armed(home/away)/unarmed
		    -- e.g. on armed: start base VerlichtingNachthal
		    if item.state == domoticz.SECURITY_ARMEDAWAY then
		        --
		        
		    elseif item.state == domoticz.SECURITY_ARMEDHOME
		        -- 
		        
		    else
		        -- item.state == domoticz.SECURITY_DISARMED
		    
		else if item.isTimer
		    if item.trigger == '10 minutes before sunset'
		        domoticz.devices('AllesUit').switchOn()
		        domoticz.devices('VerlichtingEetplaatsTafel').switchOn().afterSec(2)
		    end
		else
		    -- should not be here, as script only is triggered on timer/security
		end
		
		
		if domoticz.security == domoticz.SECURITY_ARMEDAWAY then
		    
		    
		        
		        
		    elseif domoticz.time.matchesRule('at 21:50') -- Bedtijd
		        local rnd =  math.random(15,30)
		        local rnd2 = math.random(0,10)
		        domoticz.devices('VerlichtingNachthal').switchOn().afterMin(rnd-math.random(0,10))
		        domoticz.devices('AllesUit').switchOn().afterMin(rnd)
		        --
		        
		    elseif domoticz.time.matchesRule('10 minutes before sunset')
		        --
		    
            else
               ...
            end
		    
		end
	end
}


--[[
if device.name == 'Raam berging (open)' then
            domoticz.devices('Raam berging').open()
            -- https://www.domoticz.com/wiki/DzVents:_next_generation_Lua_scripting
            domoticz.notify(
                device.name .. ': ' .. dateString,
                device.name .. ': ' .. dateString,
                PRIORITY_NORMAL,
                SOUND_DEFAULT,
                nil,
                NSS_PUSHOVER,
                nil)
        elseif (device.name == 'Raam berging (dicht)') then
            domoticz.devices('Raam berging').close()
        end
--]]