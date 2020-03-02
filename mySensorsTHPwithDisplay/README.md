# Temperature, humidity & barometric pressure node with display & mySensors network repeater

## Goal

- Measure temperature/humidity & send this to a controller using the mySensors network
- Display
 - Temperature / humidity measured locally
 - **TODO** Info obtained from remote sensors (Temperature / humidity / doorswitches / gas flow, water flow, solar power, ...) (e.g. switch screens when pushing button).
 - **TODO** Play sound when certain events are received (e.g. door x open/closed), configurable sound
 - MySensors repeater
 
## Hardware

### Components

- Arduino Nano (or Arduino Pro Mini)
- LCD display 20x4 with i2c interface
- Dallas DS18B20 temperature sensor
- Bosh BME280 temperature, humidity & pressure sensor
- NRF2401-PA-LNA wireless module (2.4Ghz) with 100µF capacitor soldered between VCC (3.3V) & GND PIN
- Module for providing stable 3.3V power to NRF2401 (3.3 from Arduino cannot provide sufficient current)

### Pinout

- D0 - TX1 (used for programming / serial output to PC)
- D1 - RX1 (used for programming / serial output to PC)
- D2 - *FREE* (interrupt capable) //Optionally NRF24L01 IRQ, only required if MY_RX_MESSAGE_BUFFER_FEATURE is defined. Recommended for high traffic nodes or gateways. Requires some more memory.
- D3 - *FREE* (interrupt capable)
- D4 - Dallas OneWire (DS18B20)
- D5 - Software Serial TX DFPlayer
- D6 - Software Serial RX DFPlayer
- D7 - *FREE*
- D8 - *FREE* 
- D9 - NRF24L01 CE
- D10 - NRF24L01 CSN/CS
- D11 - NRF24L01 MOSI
- D12 - NRF24L01 MISO
- D13 - NRF24L01 SCK
- A0 - *FREE* can be used as digital input/output too
- A1 - *FREE* can be used as digital input/output too
- A2 - *FREE* can be used as digital input/output too
- A3 - *FREE* can be used as digital input/output too
- A4 - I2C SDA (LCD + BME280) - **TODO check signal with scope, add pull-up if required** - Via level shifter 3.3-5V*
- A5 - I2C SCL (LCD + BME280) - **TODO check signal with scope, add pull-up if required** - Via level shifter 3.3-5V*
- A6 - *FREE* can be used as digital input/output too
- A7 - *FREE* can be used as digital input/output too
- 3V3- NRF24L01-VCC / BME280-VCC
- GND- NRF24L01-GND

*BME280 devices I own are 3.3V devices. Arduino Nano GPIO operate at 5V. While connecting GPIO's directly to SDA/SCA works most of the time, I noticed long-term instability (I2C bus getting locked & reboot required).
=> Either use a Arduini Pro Mini @ 3.3V, or use a level shifter.

 
