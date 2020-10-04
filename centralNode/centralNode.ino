/*   Copyright Pieter Coppens - 2019
*/

//MySensors Gateway (based on sample sketch)
// Enable debug prints to serial monitor
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24

// Enable gateway ethernet module type
#define MY_GATEWAY_W5100

// Enable Soft SPI for NRF radio (note different radio wiring is required)
// The W5100 ethernet module seems to have a hard time co-operate with
// radio on the same spi bus.
#if !defined(MY_W5100_SPI_EN) && !defined(ARDUINO_ARCH_SAMD)
#define MY_SOFTSPI
#define MY_SOFT_SPI_SCK_PIN 14
#define MY_SOFT_SPI_MOSI_PIN 15
#define MY_SOFT_SPI_MISO_PIN 16
#endif

// When W5100 is connected we have to move CE/CSN pins for NRF radio
#ifndef MY_RF24_CE_PIN
#define MY_RF24_CE_PIN 17
#endif
#ifndef MY_RF24_CS_PIN
#define MY_RF24_CS_PIN 18
#endif

// Renewal period if using DHCP
#define MY_IP_RENEWAL_INTERVAL 60000

// The port to keep open on node server mode / or port to contact in client mode
#define MY_PORT 5003
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x00}; //Arduino in Domoticz case (technische ruimte)

//Congigure channel (frequency), transmission power & speed. Lower speed => higher range
#define MY_RF24_DATARATE (RF24_1MBPS) // RF24_1MBPS for 1Mbps / RF24_2MBPS for 2Mbps // @note nRF24L01, BK2401, BK2421, BK2491 and XN297 does not support RF24_250KBPS
#define MY_RF24_PA_LEVEL (RF24_PA_MAX) // RF24 PA level for sending msgs // RF24_PA_MIN = -18dBm; RF24_PA_LOW = -12dBm; RF24_PA_HIGH = -6dBm; RF24_PA_MAX = 0dBm
#define MY_RF24_CHANNEL (85) //Set to 2485Mhz, to be outside of common used Wifi 2.4Ghz Bands & outside of medical device range
// https://en.wikipedia.org/wiki/List_of_WLAN_channels#2.4_GHz_(802.11b/g/n/ax)
// https://www.bipt.be/en/operators/radio/frequency-management/frequency-plan/table

// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
//#define MY_INCLUSION_BUTTON_FEATURE
// Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_DURATION 60
// Digital pin used for inclusion mode button
//#define MY_INCLUSION_MODE_BUTTON_PIN  3

// Set blinking period
#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Flash leds on rx/tx/err
// Uncomment to override default HW configurations
//#define MY_DEFAULT_ERR_LED_PIN 7  // Error led pin
//#define MY_DEFAULT_RX_LED_PIN  8  // Receive led pin
//#define MY_DEFAULT_TX_LED_PIN  9  // Transmit led pin

//Pieters defines
#define CHILD_ID_SOLAR 1
#define CHILD_ID_WATER 2
#define CHILD_ID_GAS 3
#define CHILD_ID_PERSISTED_CONFIG 4

#define SOLAR_PIN 19
#define WATER_PIN 20
#define GAS_PIN 21

#define PULSE_FACTOR_SOLAR 1000 // Number of pulses/kWH
#define PULSE_FACTOR_GAS   100  // 1 pulse = 0.01m³, 
#define PULSE_FACTOR_WATER 2000 // 2000 pulses per m³ //https://www.elster.nl/downloads/kent_6961_Elster_V100_04_07_03.pdf

#define SOLAR_POWER_MAX 6000  //Watt
#define WATER_FLOW_MAX 40     //liter/min
#define GAS_FLOW_MAX 100      //liter/min (meter can max handle 6m3/h)

#include <Ethernet.h>
#include <MySensors.h>
#include <SPI.h>
#include <aREST.h>
#include <avr/wdt.h>

//Global variables

long togglePinDelays[55]; //array of 55 ints intended to keep track of time when output was set (asynchronous delay per PIN)
EthernetServer server = EthernetServer(80); //Ethernet server listening on TCP port 80
aREST rest = aREST();
volatile unsigned long solarPulseCounter = 0;
volatile unsigned long waterPulseCounter = 0;
volatile unsigned long gasPulseCounter = 0;
volatile unsigned long solarPulseLatestTimeStamp = 0; //For debouncing, ignore interrupts if they come too soon
volatile unsigned long waterPulseLatestTimeStamp = 0; //For debouncing, ignore interrupts if they come too soon
volatile unsigned long gasPulseLatestTimeStamp = 0;   //For debouncing, ignore interrupts if they come too soon
volatile unsigned long previousSolarPulseCounter = 0;
volatile unsigned long previousWaterPulseCounter = 0;
volatile unsigned long previousGasPulseCounter = 0;
volatile unsigned long solarPower = 0;
double waterFlow = 0; //l/min
double gasFlow = 0;   //dm³/min
unsigned long debounce_time_solar = 80; //debounce time in ms - according to spec, pulse time is 50ms
unsigned long debounce_time_water = 500; //debounce time in ms
unsigned long debounce_time_gas = 500;   //debounce time in ms
volatile unsigned long controllerUpdateTimeStamp = 0;

class Sensor{
  public:
    int pin;
    int mysensors_child_id;
    mysensors_sensor_t mysensors_type;
    mysensors_data_t mysensors_variable_type;
    int state=0;
    int previous_state=99;
    unsigned long latestChangeTimeStamp = 0;

    Sensor(int pin, int mysensors_child_id, mysensors_sensor_t mysensors_sensor_type, mysensors_data_t mysensors_variable_type){
      pin = pin;
      mysensors_child_id = mysensors_child_id;
      mysensors_sensor_type = mysensors_sensor_type;
      mysensors_variable_type = mysensors_variable_type;
    }
    void presentToMySensors(){
      present(mysensors_child_id, mysensors_type);
    }

    void sendToController(){    
      MyMessage msg(mysensors_child_id, mysensors_variable_type);
      send(msg.set(state));
    }
};

Sensor PIR_1 = Sensor(2, 5, S_MOTION, V_TRIPPED);
Sensor PIR_2 = Sensor(3, 6, S_MOTION, V_TRIPPED);
Sensor PIR_3 = Sensor(4, 7, S_MOTION, V_TRIPPED);
Sensor PIR_4 = Sensor(5, 8, S_MOTION, V_TRIPPED);
Sensor PIR_5 = Sensor(6, 9, S_MOTION, V_TRIPPED);
Sensor POORT_BLB = Sensor(10, 10, S_DOOR, V_TRIPPED);
Sensor POORT_PAM = Sensor(11, 11, S_DOOR, V_TRIPPED);

Sensor wiredSensors[] = {PIR_1, PIR_2, PIR_3, PIR_4,PIR_5, POORT_BLB,POORT_PAM};

long updateInterval = 300; //Minimum time between sending updates for local sensors to the controller

MyMessage solar_power_msg(CHILD_ID_SOLAR, V_WATT);
MyMessage solar_kWh_msg(CHILD_ID_SOLAR, V_KWH);
MyMessage solar_pulsecount_msg(CHILD_ID_SOLAR, V_VAR1);

MyMessage gas_flow_msg(CHILD_ID_GAS, V_FLOW);
MyMessage gas_volume_msg(CHILD_ID_GAS, V_VOLUME);
MyMessage gas_pulsecount_msg(CHILD_ID_GAS, V_VAR1);

MyMessage water_flow_msg(CHILD_ID_WATER, V_FLOW);
MyMessage water_volume_msg(CHILD_ID_WATER, V_VOLUME);
MyMessage water_pulsecount_msg(CHILD_ID_WATER, V_VAR1);
MyMessage update_interval_msg(CHILD_ID_PERSISTED_CONFIG, V_VAR1);

// Declare functions to be exposed to the Rest API
int digitalPulse(String command);
int setSolarValue(String command);
int setWaterValue(String command);
int setGasValue(String command);
int setUpdateInterval(String command);
void checkPulseDelays();

//Implementation of REST API functions
int digitalPulse(String command) {
  int pin = command.toInt(); // Get pin from command
  //digitalWrite(pin, !digitalRead(pin));
  digitalWrite(pin, 0); //Switch on (inversed logic due to relay board)
  togglePinDelays[pin] = millis();
  return 0;
}

int setSolarValue(String command) {  
  unsigned long arest_solar_meter_value = 0;  //Up to the last digit on meter (0.1kWh)
  arest_solar_meter_value = strtoul(command.c_str(), NULL, 10);
  if (arest_solar_meter_value != 0) {
    unsigned long wattHours = arest_solar_meter_value * 100; //Meter has resolution of 0.1 kWh, x100 to get to Wh
    solarPulseCounter = wattHours * (PULSE_FACTOR_SOLAR / 1000);
    previousSolarPulseCounter = 0; //To ensure next update is sent on loop()
  }
  return 0;
}

int setWaterValue(String command) {
  unsigned long arest_water_meter_value = 0;  //Up to last digit on meter (0.1 liter)  
  arest_water_meter_value = strtoul(command.c_str(), NULL, 10); //returns 0 if not successful, value in x deciliter otherwise
  if (arest_water_meter_value != 0) {
    unsigned long liters = arest_water_meter_value / 10; //Meter gives value in deciliter
    waterPulseCounter = liters * (PULSE_FACTOR_WATER / 1000);
    previousWaterPulseCounter = 0; //To ensure next update is sent on loop()
  }
  return 0;
}

int setGasValue(String command) {
  unsigned long arest_gas_meter_value = 0;    //Up to last digit on meter (1 dm³)  
  arest_gas_meter_value = strtoul(command.c_str(), NULL, 10); //returns 0 if not successful
  if (arest_gas_meter_value != 0) {
    unsigned long liters = arest_gas_meter_value; //Meter gives value in dm3 (liter)
    gasPulseCounter = liters * (PULSE_FACTOR_GAS / 1000.0);
    previousGasPulseCounter = 0; //To ensure next update is sent on loop()
  }
  return 0;
}

int setUpdateInterval(String command){
  //Set interval to send updates to the controller (value in seconds)
  if (command.toInt() > 15){ //Ensure update interval >=15s to avoid spamming gateway/controller
    updateInterval = command.toInt();
    send(update_interval_msg.set(updateInterval)); //Persist value on controller
    }
   return 0;
} 

void checkPulseDelays() {
  //Used as non-blocking sleep alternative for the digitalPulse (keep track when to toggle output value again).
  unsigned long now = millis();
  for (int pin = 22; pin <= 37; pin++) { //Only checking PINs connected to relays to save time
    if (togglePinDelays[pin] != 0) {
      if ((now - togglePinDelays[pin]) > 400) { //400ms
        //digitalWrite(pin, !digitalRead(pin));
        digitalWrite(pin, 1); //Switch off (inversed logic due to relay board)
        togglePinDelays[pin] = 0;
      }
    }
  }
}

void presentation()
{
  sendSketchInfo("MySensors Gateway", "1.0");
  present(CHILD_ID_SOLAR, S_POWER); //V_WATT, V_KWH, V_VAR1, V_VA, V_POWER_FACTOR
  present(CHILD_ID_WATER, S_WATER); //V_FLOW, V_VOLUME
  present(CHILD_ID_GAS, S_GAS);     //V_FLOW, V_VOLUME
  present(CHILD_ID_PERSISTED_CONFIG, S_CUSTOM); //Used to persist config info, such as updateInterval (and maybe afterwards extend to debounce time for solar, water, gas).

  for (int idx=0; idx < 7; idx++){
    wiredSensors[idx].presentToMySensors();
  }
}

void isr_solar_pulse() {  
  unsigned long currentTimeStamp = (unsigned long)micros();
  unsigned long interval = currentTimeStamp - solarPulseLatestTimeStamp;  
  if (interval >= (debounce_time_solar * 1000)) {    
    solarPulseCounter += 1;
    solarPulseLatestTimeStamp = currentTimeStamp;
    //Solar power calculation (average power of timeframe between 2 pulses)
    unsigned long currentPower = 3600000000.0/interval; //in case 1 pulse per Wh.    
    if (currentPower <= SOLAR_POWER_MAX) { //filter out outliers
      solarPower = currentPower;
    }
  }
}

void isr_water_pulse() {  
  unsigned long currentTimeStamp = (unsigned long)micros();
  unsigned long interval = currentTimeStamp - waterPulseLatestTimeStamp;
  if (interval >= (debounce_time_water * 1000)) {    
    waterPulseCounter += 1;
    waterPulseLatestTimeStamp = currentTimeStamp;
    double currentWaterFlow = (1000.0 / PULSE_FACTOR_WATER) / (interval / 60000000.0); //liter/min
    if (currentWaterFlow < WATER_FLOW_MAX) { //filter out outliers
      waterFlow = currentWaterFlow;
    }
  }
}

void isr_gas_pulse() {
  unsigned long currentTimeStamp = (unsigned long)micros();
  unsigned long interval = currentTimeStamp - gasPulseLatestTimeStamp;
  if (interval >= (debounce_time_gas * 1000)) {    
    gasPulseCounter += 1;
    gasPulseLatestTimeStamp = currentTimeStamp;
    //1 Pulse = 10 liter, want gas flow in liter/min
    double currentGasFlow = (1000.0 / PULSE_FACTOR_GAS) / (interval / 60000000.0); //liter/min
    if (currentGasFlow < GAS_FLOW_MAX) { //filter out outliers
      gasFlow = currentGasFlow;
    }
  }
}

void setup(void)
{
  pinMode(SOLAR_PIN, INPUT_PULLUP);
  pinMode(WATER_PIN, INPUT_PULLUP);
  pinMode(GAS_PIN, INPUT);
  
  //set PINmodes for connected relays + set value to high (inverted logic in relay)
  for (int pin = 22; pin <= 37; pin++) {
    pinMode (pin, OUTPUT);
    digitalWrite(pin, HIGH);
  }
  for (int idx=0; idx < 7; idx++){
    pinMode(wiredSensors[idx].pin, INPUT_PULLUP);    
  }

  //initialize values in togglePinDelays
  for (int pin = 22; pin <= 37; pin++) {
    togglePinDelays[pin] = 0;
  }
  // REST interface: Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("101010");
  rest.set_name((char*)"ArduinoDomoticz");
  rest.function((char*)"pulse", digitalPulse); // register function to REST interface
  rest.function((char*)"setSolarValue", setSolarValue);
  rest.function((char*)"setWaterValue", setWaterValue);
  rest.function((char*)"setGasValue", setGasValue);
  rest.function((char*)"setUpdateInterval", setUpdateInterval);  
  rest.variable("solar",&solarPulseCounter);
  rest.variable("water",&waterPulseCounter);
  rest.variable("gas",&gasPulseCounter);
  rest.variable("updateInterval", &updateInterval); //Update interval in seconds


  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
  }
  
  server.begin();
  // Start watchdog with 8s timeout
  wdt_enable(WDTO_8S);

  /*
  Serial.println("Initializing counter values for solar, water & gas. Getting last known values from controller, meanwhile you can use the REST interface as well");
  Serial.print("- e.g. http://"); Serial.print(Ethernet.localIP());Serial.println("/setSolarValue?params=429825");
  Serial.print("- e.g. http://"); Serial.print(Ethernet.localIP());Serial.println("/setWaterValue?params=3480649");
  Serial.print("- e.g. http://"); Serial.print(Ethernet.localIP());Serial.println("/setGasValue?params=9498845");
  */
  
  while (solarPulseCounter == 0 || waterPulseCounter == 0 || gasPulseCounter == 0){
    EthernetClient client = server.available();
    rest.handle(client); //ensure REST interface works while waiting for answer from controller
    checkPulseDelays();  //ensure REST interface works while waiting for answer from controller
    wdt_reset();
    requestCounterValuesFromController();
  }
  //Request persisted configuration settings from the controller
  request(CHILD_ID_PERSISTED_CONFIG, V_VAR1); //Update interval (seconds)
  wait(1000);  
  attachInterrupt(digitalPinToInterrupt(SOLAR_PIN), isr_solar_pulse, FALLING);
  attachInterrupt(digitalPinToInterrupt(WATER_PIN), isr_water_pulse, FALLING);
  attachInterrupt(digitalPinToInterrupt(GAS_PIN), isr_gas_pulse, FALLING);
}

void requestCounterValuesFromController(){  
  if (solarPulseCounter == 0) {
    //Serial.println("Requesting SOLAR pulse count from controller");
    request(CHILD_ID_SOLAR, V_VAR1);
    wait(1000);
	wdt_reset();
  }
  if (waterPulseCounter == 0) {
    //Serial.println("Requesting WATER pulse count from controller");
    request(CHILD_ID_WATER, V_VAR1);    
    wait(1000);
	wdt_reset();
  }
  if (gasPulseCounter == 0) {
    //Serial.println("Requesting GAS pulse count from controller");
    request(CHILD_ID_GAS, V_VAR1);
    wait(1000);
	wdt_reset();
  }
}

void receive(const MyMessage &message)
//Handle reception of MySensors message (expecting values from gateway requested in setup)
{
  
  if ((message.sensor == CHILD_ID_PERSISTED_CONFIG) && (message.type==V_VAR1)){
      //Made the convestion to store updateInterval in V_VAR1 on controller. Other config can use e.g. V_VAR2...V_VARX
      if (message.getLong() >= 15) { //Do not allow update times < 15 seconds (overkill & puts too much load on system)
        updateInterval = message.getLong();
      }
  }
  
  if (message.type != V_VAR1) {
    return;
  }

  switch (message.sensor) {
    case CHILD_ID_SOLAR:
      solarPulseCounter += message.getULong();
      //Serial.print("Solar pulse count received from controller: ");
      //Serial.println(solarPulseCounter);
      break;
    case CHILD_ID_WATER:
      waterPulseCounter += message.getULong();
      //Serial.print("Water pulse count received from controller: ");
      //Serial.println(waterPulseCounter);
      break;
    case CHILD_ID_GAS:
      gasPulseCounter += message.getULong();
      //Serial.print("Gas pulse count received from controller: ");
      //Serial.println(gasPulseCounter);
      break;
  }
}

void handleMotionAndDoorSwitches(){
  for (int idx=0; idx < 7; idx++){    
    wiredSensors[idx].state = digitalRead(wiredSensors[idx].pin);
    if (wiredSensors[idx].state != wiredSensors[idx].previous_state){
      if ((millis() - wiredSensors[idx].latestChangeTimeStamp) < 100){
         //debounce
      }
      else{
        wiredSensors[idx].latestChangeTimeStamp = millis();
        wiredSensors[idx].previous_state = wiredSensors[idx].state;
        wiredSensors[idx].sendToController();
      }
    }
  }
}

void loop() {
  unsigned long now = millis();
  EthernetClient client = server.available();
  rest.handle(client);
  checkPulseDelays();
  handleMotionAndDoorSwitches();  
  if ((now - controllerUpdateTimeStamp) >= (unsigned long)updateInterval*1000) { //Send values for local sensors to controller max every updateInterval
    
    //Set flow to 0 for the various sensors after not getting an update for x time (otherwise flow/power message would be stuck to last known value calculated in the ISR.
    if (((unsigned long) micros() - waterPulseLatestTimeStamp) > 600000000) { //10 minute
      waterFlow = 0;
    }
    if (((unsigned long) micros() - solarPulseLatestTimeStamp) > 600000000) { //10 minutes
      solarPower = 0;
    }
    if (((unsigned long) micros() - gasPulseLatestTimeStamp) > 600000000) {   //10 minutes
      gasFlow = 0;
    }

    send(solar_power_msg.set(solarPower));
    send(water_flow_msg.set(waterFlow, 2));
    send(gas_flow_msg.set(gasFlow, 2));

    if (solarPulseCounter > previousSolarPulseCounter) {
      previousSolarPulseCounter = solarPulseCounter;
      double kWh = ((double)solarPulseCounter / ((double)PULSE_FACTOR_SOLAR));
      send(solar_pulsecount_msg.set(solarPulseCounter));  // Send pulse count value to gw
      send(solar_kWh_msg.set(kWh, 3));  // Send kWh value to gw
      }

    if (waterPulseCounter > previousWaterPulseCounter) {
      previousWaterPulseCounter = waterPulseCounter;
      double totalWaterConsumption = waterPulseCounter / double(PULSE_FACTOR_WATER); //in liter
      send(water_pulsecount_msg.set(waterPulseCounter));
      send(water_volume_msg.set(totalWaterConsumption, 4));
    }

    if (gasPulseCounter > previousGasPulseCounter) {
      previousGasPulseCounter = gasPulseCounter;
      double totalGasConsumption = gasPulseCounter / (double)PULSE_FACTOR_GAS; //in m³
      send(gas_volume_msg.set(totalGasConsumption, 4));
      send(gas_pulsecount_msg.set(gasPulseCounter));
    }
    controllerUpdateTimeStamp = now;
  }
  wdt_reset();
}
