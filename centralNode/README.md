# Purpose of this sketch

Sketch written for an Arduino Mega2560 as part of my domotics. This sketch serves the below purposes:
1. MySensors network gateway (https://www.mysensors.org/)
2. Measuring Electricity/Gas/Water/Solar consumption/generation, reporting to a controller via a mySensors network.

   As this sketch is building on top of MySensors, it is compatible with a mutlitude of domotics solutions as such as Domoticz, HomeGenie, Home Assistant, Mozilla WebThings, MyController.org, MyNotes.net, nodeRed; OpenHAB, VERA, ...   
3. REST interface to control Arduino I/O & set set/request values for the Pulse counters. Using this to control my lights etc.
   Based on https://github.com/marcoschwartz/aREST

# Hardware setup

1. Ethernet Shield (W5100) connected to the Mega2560 (for the REST interface & communication with a Mysensors compatible controller
2. PIN 22->37 are connected to a 16-channel relay board (which in their turn control my teleruptors).
3. For pulse counting - the pins below have been configured
   - PIN 19:  Configured as input with internal pull-up to VCC (5V) => Connected to S0 (open collector interface of energy meter)
   - PIN 20:  Configured as input with internal pull-up to VCC (5V) => Connected to reed switch on water meter
   - PIN 21:  Configured as input - planing to use a hall-effect sensor with digital output to count gas pulses (as magnet in meter
              is too weak to activate a reed switch)
4. a NRF24 (NFR24-PA-LNA) module for wireless communication (MySensors):
   Connecting the NRF24L01 :
   - PIN 14 NRF module SOFT SPI SCK
   - PIN 15 NRF module SOFT SPI MOSI
   - PIN 16 NRF module SOFT SPI MISO
   - PIN 17 NRF module SOFT SPI CE
   - PIN 18 NRF module SOFT SPI CS
   - !! Provide a stable 3.3V to the module & solder a capacitor on the 3.3V supply (>=200µF as close as possible to the
    module)
    
# RF configuration (hard-coded in sketch)

Important: ensure all participants of the MySensors network are on same channel & datarate.
This sketch has the configuration settings below:
- #define MY_RF24_DATARATE (RF24_1MBPS) // RF24_1MBPS for 1Mbps / RF24_2MBPS for 2Mbps // @note nRF24L01, BK2401, BK2421, BK2491 and XN297 does not support RF24_250KBPS
- #define MY_RF24_PA_LEVEL (RF24_PA_MAX) // RF24 PA level for sending msgs // RF24_PA_MIN = -18dBm; RF24_PA_LOW = -12dBm; RF24_PA_HIGH = -6dBm; RF24_PA_MAX = 0dBm
- #define MY_RF24_CHANNEL (85) //Set to 2485Mhz, to be outside of common used Wifi 2.4Ghz Bands & outside of medical device range:
    - https://en.wikipedia.org/wiki/List_of_WLAN_channels#2.4_GHz_(802.11b/g/n/ax)
    - https://www.bipt.be/en/operators/radio/frequency-management/frequency-plan/table
      
# Initialisation of pulse counters
 
After sketch startup, the sketch will request the controller for the last known values of the solar/water/gas pulse counters.
Meanwhile (and at any time), it is possible to set those values using a REST interface (useful in case the controller (i.e. domoticz) would run 'behind' or when it has no values stored yet.
The sketch will only start counting as soon as all values are initialised.

- http://ip-address/setSolarValue?params=429825  => Take ALL digits from meter (except leading zeroes)
- http://ip-address/setWaterValue?params=3480649 => Take ALL digits from meter (except leading zeroes)
- http://ip-address/setGasValue?params=9498845   => Take ALL digits from meter (except leading zeroes)
- http://ip-address/setUpdateInterval?params=30 => Set controller update interval to 30 seconds.

# Other details on REST interface

1.  REST interface provices a method to generate a pulse on a PIN.
    This is used to control the teleruptors of my electricity installation.
    http://ip-address/pulse?params=34; Generate a single pulse of 250ms on PIN specified in params (will make my teleruptors toggle)

2.  Request the current values of the different counters via REST:
      http://ip-address
        =>  {"variables": {"solar": 42982577, "water": 696128, "gas": 959884}, "id": "101010", "name": "ArduinoDomoticz",
            "hardware": "arduino", "connected": true}
   
3.  Some other functionalities provided by the arest library:
    - http://ip-address/mode/6/i => Set PIN 6 as inpout (o for output)
    - http://ip-address/analog/5/10 => Analog output to PIN 5 using PWM - value 10 (0-255) (only works for PWM pins) 
    - http://ip-address/digital/6/1 => Digital output to PIN 6, value 1
    - http://ip-address/digital/20 => reads value from pin number 20 in JSON format
      e.g. {"return_value": 1, "id": "101010", "name": "ArduinoDomoticz", "hardware": "arduino", "connected": true}
    - http://ip-address/analog/0 => returns analog value from pin number A0 in JSON format:
      e.g. {"return_value": 503, "id": "101010", "name": "ArduinoDomoticz", "hardware": "arduino", "connected": true}
      
## Future extension may include

1. Configure debounce timeout via REST
2. Configure RF24 channel, datarate & PA level via REST
