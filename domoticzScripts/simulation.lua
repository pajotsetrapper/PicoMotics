return {
    
	on = {
		timer = {
			-- 'every minute',	-- uses sunset/sunrise info from Domoticz
			'10 minutes before sunset', --zon uit, licht aan
			'every 30 minutes between sunset and 22:00', -- to the loo
			'every hour between sunset and 22:00', -- berging
			'every 2 hours between sunset and 22:00', -- keldertrap
			'at 20:50', -- kids to bed
			'at 22:00', -- up we go
			'every 2 hours between 23:00 and sunrise', -- tijdens de nacht
		},
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
		
		if (item.isSecurity) then
		    -- code to handle case where system being armed(home/away)/disarmed
		    -- e.g. on armed: start base VerlichtingNachthal
		    
		    if (item.state == domoticz.SECURITY_ARMEDAWAY) then
		        domoticz.log('Security event was triggered: system armed-away', domoticz.LOG_INFO)
    	        domoticz.devices('AllesUit').switchOn()
    	        if domoticz.time.matchesRule('between sunset and 21:55') then
    	            domoticz.devices('VerlichtingEetplaatsTafel').switchOn().afterMin(math.random(0,10))
    	        end
    	        
		    elseif (item.state == domoticz.SECURITY_ARMEDHOME) then
		        domoticz.log('Security event was triggered: system armed-home', domoticz.LOG_INFO)
		        domoticz.devices('AllesUit').switchOn() --mabe too harsh, however ensures clean state when stopping simulation
		        
		    else
		        domoticz.log('Security event was triggered: system disarmed', domoticz.LOG_INFO)
		        domoticz.devices('AllesUit').switchOn() --mabe too harsh, however ensures clean state when stopping simulation
		    end
		    
		elseif (item.isTimer) then
		    -- handle the timer events
		    
		    if (domoticz.security == domoticz.SECURITY_ARMEDAWAY) then
		        
		        if (item.trigger == '10 minutes before sunset') then
		            domoticz.log('Timer event was triggered: alles uit + VerlichtingEetplaatsTafel aan', domoticz.LOG_INFO)
    		        domoticz.devices('AllesUit').switchOn() -- clean state
    		        domoticz.devices('VerlichtingEetplaatsTafel').switchOn().afterSec(2)
    		        
		        elseif (item.trigger == 'every 30 minutes between sunset and 22:00') then
		            -- toilet sim licht toilet tussen 1 & 5 min aan
		            domoticz.log('Timer event was triggered: toilet-simulatie', domoticz.LOG_INFO)
		            domoticz.devices('VerlichtingToilet').switchOn()
		            domoticz.devices('VerlichtingToilet').switchOff().withinMin(math.random(1,5))
		            
		        elseif (item.trigger == 'every hour between sunset and 22:00') then
		            domoticz.log('Timer event was triggered: berging-simulatie', domoticz.LOG_INFO)
		            domoticz.devices('VerlichtingBerging').switchOn()
		            domoticz.devices('VerlichtingBerging').switchOff().withinMin(math.random(1,2))
		            
		        elseif  (item.trigger == 'every 2 hours between sunset and 22:00') then -- keldertrap'
		            domoticz.log('Timer event was triggered: keldertrap-simulatie', domoticz.LOG_INFO)
		            domoticz.devices('VerlichtingKeldertrap').switchOn()
		            domoticz.devices('VerlichtingKeldertrap').switchOff().withinMin(math.random(1,2))
	            
                elseif (item.trigger == 'at 20:50') then
                    domoticz.log('Timer event was triggered: kids-to-bed-simulatie', domoticz.LOG_INFO)
                    domoticz.devices('VerlichtingNachthal').switchOn()
                    domoticz.devices('VerlichtingSLKT').switchOn().afterSec(30)
                    domoticz.devices('VerlichtingSLKN').switchOn().afterSec(20)
                    domoticz.devices('VerlichtingNachthal').switchOff().withinMin(math.random(3,10))
                    domoticz.devices('VerlichtingSLKN').switchOff().withinMin(math.random(5,10))
                    domoticz.devices('VerlichtingSLKT').switchOff().withinMin(math.random(5,10))
                    
                elseif (item.trigger == 'at 22:00') then
                    domoticz.log('Timer event was triggered: bedtijd-simulatie', domoticz.LOG_INFO)
                    local variability = math.random(0,15)
                    domoticz.devices('VerlichtingNachthal').switchOn().afterMin(variability).forMin(2)
                    domoticz.devices('VerlichtingEetplaatsTafel').switchOff().afterMin(variability)
                    domoticz.devices('VerlichtingSLKPN').switchOn().afterMin(variability+1).forMin(math.random(5,10))
                    domoticz.devices('AllesUit').switchOn()
                    
                elseif (item.trigger == 'every 2 hours between 23:00 and sunrise') then
                    domoticz.log('Timer event was triggered: nacht-pipi simulatie', domoticz.LOG_INFO)
                    domoticz.devices('VerlichtingNachthal').switchOn().afterMin(math.random(0,30)).forMin(1)
                    
                else
	                -- Well, does not matter
	            end
		    else
		        -- Someone is home (SECURITY_ARMEDHOME or SECURITY_DISARMED)
		    end
		else
		    -- should not be here, as script only is triggered on timer/security
		end
	end
}
