-- 
-- Context - built a smart air humidifier around ESPEasy.
-- Using a REST-interface, the current humidity needs to be sent to the humidifier.
-- The humidifier will use this value for controlling a heater.
-- This humidity is being sent using Domoticz.
--

return {
    logging = {
        --level = domoticz.LOG_DEBUG, -- Uncomment to override the dzVents global logging setting and use debug
        marker = 'Humidifier'
    },
	on = {
		timer = {
			'every 1 minutes' -- just an example to trigger the request
		},
		httpResponses = {
			'onHumidifierResponse' -- must match with the callback passed to the openURL command
		}
	},
	execute = function(domoticz, item)

        local humidifierURL = 'http://ESP8266Humidifier.lan/control?cmd=TaskValueSet,2,4,'
	-- Dummy device set on task 2. Value 4 = humidity

		if (item.isTimer) then
		    --local temperature = domoticz.devices('Klimaat gelijkvloers').temperature
		    local humidity = domoticz.round(domoticz.devices('Klimaat gelijkvloers').humidity, 0)


			domoticz.openURL({
                                url = humidifierURL..humidity,
                                method = 'GET',
                                callback = 'onHumidifierResponse',
            })
		end

		if (item.isHTTPResponse) then
			if (item.statusCode ~= 200) then
				domoticz.log('Could not send humidity to humidifier', domoticz.LOG_ERROR)
				domoticz.log(item, domoticz.LOG_ERROR)
			end
		end
	end
}

