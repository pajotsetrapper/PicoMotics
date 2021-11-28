return {
    
	on = {
		timer = {
			-- 'every minute',	-- uses sunset/sunrise info from Domoticz
			'10 minutes before sunset', --zondondergang
			'every 20 minutes between sunset and 22:00', -- zonsondergang tot slapen
			'every 40 minutes between sunset and 22:00', -- for restroom sim
			'every 60 minutes between sunset and 22:00', -- berging
			'every 10 minutes between 20:50 and 21:05',  -- kids to bed
			
			
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
		    -- code to handle case where system being armed(home/away)/disarmed
		    -- e.g. on armed: start base VerlichtingNachthal
		    
		    if item.state == domoticz.SECURITY_ARMEDAWAY then
    	        domoticz.devices('AllesUit').switchOn()
    	        if domoticz.time.matchesRule('between sunset and 21:55') then
    	            domoticz.devices('VerlichtingEetplaatsTafel').switchOn().afterMin(math.random(0,10))
    	        end
    	        
		    elseif item.state == domoticz.SECURITY_ARMEDHOME
		        domoticz.devices('AllesUit').switchOn() --mabe too harsh
		        
		    else
		        domoticz.devices('AllesUit').switchOn() --mabe too harsh
		    end
		    
		else if item.isTimer
		    if domoticz.SECURITY_ARMEDAWAY then
		        if item.trigger == '10 minutes before sunset' then
    		        domoticz.devices('AllesUit').switchOn()
    		        domoticz.devices('VerlichtingEetplaatsTafel').switchOn().afterSec(2)
    		        
		        else if item.trigger == 'every 40 minutes between sunset and 22:00' then
		            -- toilet sim licht toilet tussen 1 & 5 min aan
		            domoticz.devices('VerlichtingToilet').switchOn()
		            domoticz.devices('VerlichtingToilet').switchOff().withinMin(math.random(1,5))
		            
		        else if item.trigger == 'every 60 minutes between sunset and 22:00'
		            domoticz.devices('VerlichtingBerging').switchOn()
		            domoticz.devices('VerlichtingBerging').switchOff().withinMin(math.random(1,2))
	            
                else if item.trigger == 'every 10 minutes between 20:50 and 21:05'
                    domoticz.devices('VerlichtingNachthal').switchOn()
                    domoticz.devices('VerlichtingSLKT').switchOn()
                    domoticz.devices('VerlichtingSLKN').switchOn().afterSec(10)
                    domoticz.devices('VerlichtingNachthal').switchOff().withinMin(math.random(3,10))
                    domoticz.devices('VerlichtingSLKN').switchOff().withinMin(math.random(5,15))
                    domoticz.devices('VerlichtingSLKT').switchOff().withinMin(math.random(5,15))
                else
	                --
	            end
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