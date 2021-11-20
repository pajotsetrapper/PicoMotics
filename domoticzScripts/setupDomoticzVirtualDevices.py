# Python 3.x script, creating the virtual devices/sensors required for home automation project
# Consumes the Domoticz API/JSON URL's: https://www.domoticz.com/wiki/Domoticz_API/JSON_URL%27s

import requests, base64

class DomoticzException(Exception):
    pass

class Device:
    def __init__(self, name, dtype, dsubtype, idx=None, onActionURL="", offActionURL=""):
        self.name = name
        self.type = dtype
        self.subtype = dsubtype
        self.idx = idx
        self.onActionURL = onActionURL
        self.offActionURL = offActionURL

class Sensor:
    def __init__(self, name, stype, idx=None):
        self.name = name
        self.type = stype
        self.idx = idx                 

class DomoticzWrapper:
    def __init__(self, host, verifySSL=True):
        self.url = "https://{}".format(host)
        self.verifySSL = verifySSL

    def getVersion(self):
        response = requests.get(self.url + '/json.htm?type=command&param=getversion', verify=self.verifySSL)
        if response.status_code != 200:
            raise DomoticzException
        return response.json()

    def createVirtualHardware(self, hardwareType=15, port=1, name="virtualDevices"):
        requestUrl = self.url + "/json.htm?type=command&param=addhardware&htype={}&port={}&name={}&enabled=true".format(hardwareType, port, name)
        response = requests.get(requestUrl, verify=self.verifySSL)        
        if response.status_code != 200:
            raise DomoticzException
        return response.json()

    def createDevice(self, deviceIDX, sensorName, deviceType, deviceSubtype):
        """
        https://github.com/domoticz/domoticz/blob/development/hardware/hardwaretypes.h
        """
        requestUrl = self.url + '/json.htm?type=createdevice&idx={}&sensorname={}&devicetype={}&devicesubtype={}'.format(deviceIDX, sensorName, deviceType, deviceSubtype)
        response = requests.get(requestUrl, verify=self.verifySSL)
        if response.status_code != 200:
            raise DomoticzException
        return response.json()
        
    def createVirtualSensor(self, deviceIDX, sensorName, sensorType):
        requestUrl = self.url + '/json.htm?type=createvirtualsensor&idx={}&sensorname={}&sensortype={}'.format(deviceIDX, sensorName, sensorType)
        response = requests.get(requestUrl, verify=self.verifySSL)
        if response.status_code != 200:
            raise DomoticzException
        return response.json()

    def setOnOffActionStrings(self, deviceIDX, onActionURL, offActionURL):        
        encodedOnActionURLBytes = base64.b64encode(onActionURL.encode("utf-8"))
        encodedOnActionURLStr = str(encodedOnActionURLBytes, "utf-8")
        encodedOffActionURLBytes = base64.b64encode(offActionURL.encode("utf-8"))
        encodedOffActionURLStr = str(encodedOffActionURLBytes, "utf-8")
        
        requestUrl = self.url+ '/json.htm?type=setused&used=true&idx={}&strparam1={}&strparam2={}'.format(deviceIDX, encodedOnActionURLStr, encodedOffActionURLStr)
        response = requests.get(requestUrl, verify=self.verifySSL)        
        
        if response.status_code != 200:
            raise DomoticzException
        return response.json()

if __name__ == '__main__':

    pTypeGeneralSwitch = 244  #See https://github.com/domoticz/domoticz/blob/development/hardware/hardwaretypes.h
    sSwitchGeneralSwitch = 73 #See https://github.com/domoticz/domoticz/blob/development/hardware/hardwaretypes.h
    sSwitchTypeIMPULS = 5
    sTypeTHBFloat = 16 #Weather station
    TEMP_HUM_BARO = 84 #See https://www.domoticz.com/wiki/Domoticz_API/JSON_URL%27s
    

    devicesToCreate = (        
        Device('AllesUit', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=22', offActionURL='http://domoticzarduino.lan/pulse?params=22'),        
        Device('VerlichtingEetplaatsPiano', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=23', offActionURL='http://domoticzarduino.lan/pulse?params=23'),        
        Device('VerlichtingKeldertrap', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=24', offActionURL='http://domoticzarduino.lan/pulse?params=24'),        
        Device('VerlichtingInkom', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=25', offActionURL='http://domoticzarduino.lan/pulse?params=25'),        
        Device('VerlichtingToilet', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=26', offActionURL='http://domoticzarduino.lan/pulse?params=26'),        
        Device('VerlichtingBerging', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=27', offActionURL='http://domoticzarduino.lan/pulse?params=27'),        
        Device('VerlichtingBureau', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=28', offActionURL='http://domoticzarduino.lan/pulse?params=28'),        
        #Device('?????', pTypeGeneralSwitch,sSwitchTypeIMPULS, idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=29', offActionURL='http://domoticzarduino.lan/pulse?params=29'),
        Device('VerlichtingEetplaatsTafel', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=30', offActionURL='http://domoticzarduino.lan/pulse?params=30'),        
        Device('VerlichtingKeukeneilend', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=31', offActionURL='http://domoticzarduino.lan/pulse?params=31'),
        Device('VerlichtingZitplaatsRaam', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=32', offActionURL='http://domoticzarduino.lan/pulse?params=32'),
        Device('VerlichtingZitplaatsTV', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=33', offActionURL='http://domoticzarduino.lan/pulse?params=33'),
        Device('VerlichtingNachthal', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=34', offActionURL='http://domoticzarduino.lan/pulse?params=34'),
        Device('VerlichtingSLKPN', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=35', offActionURL='http://domoticzarduino.lan/pulse?params=35'),
        Device('VerlichtingSLKN', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=36', offActionURL='http://domoticzarduino.lan/pulse?params=36'),
        Device('VerlichtingSLKT', pTypeGeneralSwitch,sSwitchGeneralSwitch , idx=None, onActionURL='http://domoticzarduino.lan/pulse?params=37', offActionURL='http://domoticzarduino.lan/pulse?params=37'),
        Device('sVoordeur', pTypeGeneralSwitch,sSwitchGeneralSwitch, idx=None),
        Device('sAchterdeur', pTypeGeneralSwitch,sSwitchGeneralSwitch, idx=None),
        Device('sRaam Berging', pTypeGeneralSwitch,sSwitchGeneralSwitch, idx=None),
        Device('sSchuifraam', pTypeGeneralSwitch,sSwitchGeneralSwitch, idx=None),
        Device('sLuchtbevochtiger actief', pTypeGeneralSwitch,sSwitchGeneralSwitch, idx=None),
        Device('sLuchtbevochtiger - tank vullen', pTypeGeneralSwitch,sSwitchGeneralSwitch, idx=None),        
    )

    sensorsToCreate = (
        Sensor('Klimaat gelijkvloers', TEMP_HUM_BARO, idx=None),
    )
    
   
    hostname = "rpi2.lan"    
    domoticz = DomoticzWrapper(hostname, verifySSL=False)
    domoticz.getVersion()    
    response = domoticz.createVirtualHardware()
    virt_hw_index = response['idx']
    for device in devicesToCreate:
        response = domoticz.createDevice(virt_hw_index, device.name, device.type, device.subtype)
        device.idx = response['idx'] # store device index in Device instance
        domoticz.setOnOffActionStrings(device.idx, device.onActionURL, device.offActionURL)
        
    for sensor in sensorsToCreate:
        response = domoticz.createVirtualSensor(virt_hw_index, sensor.name, sensor.type)
        sensor.idx = response['idx']
        
