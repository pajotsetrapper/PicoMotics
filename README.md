# PicoMotics

Documentation & source code of my homebrew domotics (and beyond) project.

The goal of this project is to:
- Add intelligence to my classic teleruptor based electricity installation: ability to control lights, power sockets etc... This in a way that the classic installation still works as before in case the domotics part would fail.
- Measure & report on solar power generation, electricity, water & gas consumption
- Measure temperature / humidity & barometric pressure inside / outside the house
- Raise alarms when events happen (movement detection, smoke, door/window switches, glass break sensors, water/gas leaks, air too dry, ...)
- Control network receiver, TV, ...
- Explore Arduino programming, wireless interfaces, ...

Making use of Domoticz (running on a single board PC) as a controller, but should work with other similar software as well.
Sensor-wise, the system can communicate over the 433Mhz band (as used by many cheap devices on the market) and over a MySensors (https://www.mysensors.org/) mesh network (way more reliable, you'll need to create your own sensors though).

About the subfolders to be found in here:
- centralNode: Arduino Sketch for the REST controlled IO, Pulse counters & MySensors Gateway
- mySensorsTHPwithDisplay: Arduino sketch where a DS18b20 & BME280 measure temperature, humidity & barometric pressure & report these values using a MySensors network.
- doc: documentation
