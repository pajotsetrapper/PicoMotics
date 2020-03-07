return {
    logging = {
        --level = domoticz.LOG_DEBUG, -- Uncomment to override the dzVents global logging setting and use debug
        marker = 'PVOutput'
    },
	on = {
		timer = {
			'every 5 minutes' -- just an example to trigger the request
		},
		httpResponses = {
			'triggerPVoutput' -- must match with the callback passed to the openURL command
		}
	},
	execute = function(domoticz, item)
	-------------------------
        -- PVoutput parameters --
        -------------------------
        local PVoutputApi = 'deadbeef0102030408060708deadbeef98765432'      -- Your PVoutput api key
        local PVoutputSystemID = '12345'                                    -- Your PVoutput System ID
        local PVoutputURL = 'http://pvoutput.org/service/r2/addstatus.jsp'  -- The URL to the PVoutput Service
        ------------------------

		if (item.isTimer) then
		    local temperature = domoticz.devices('Bureau').temperature
		    local temperature_truncated = domoticz.round(temperature,2)
            local SolarEnergy  = domoticz.devices('Zonnepanelen').WhTotal -- v1 in Watt hours (accumulated) //energy generation
            local SolarPower  = domoticz.devices('Zonnepanelen').WhActual -- v2 in Watts                    //power generation
            
            -- For info on REST API: https://pvoutput.org/help.html
			domoticz.openURL({
                                url = PVoutputURL..'?d='..os.date("%Y%m%d")..'&t='..os.date("%H:%M")..'&v1='..SolarEnergy..'&v2='..SolarPower..'&c1=1',
                                method = 'GET',
                                callback = 'triggerPVoutput',
                                headers = { ['X-Pvoutput-Apikey'] = PVoutputApi, 
                                            ['X-Pvoutput-SystemId'] = PVoutputSystemID
                                          }
            })
		end

		if (item.isHTTPResponse) then
			if (item.statusCode ~= 200) then
				domoticz.log('Could not update pvoutput.org', domoticz.LOG_ERROR)
				domoticz.log(item, domoticz.LOG_ERROR)
			end
		end
	end
}
