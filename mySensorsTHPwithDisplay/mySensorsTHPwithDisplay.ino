/*
 * Pieter Coppens 2019
 */

// Enable debug prints to serial monitor
//#define MY_DEBUG
// Enable and select radio type attached
#define MY_RADIO_RF24
#define MY_REPEATER_FEATURE //Enable repeater mode

//Configure channel (frequency), transmission power & speed. Lower speed => higher range.
//These values are the same as the ones used on the gateway (mandatory!)
#define MY_RF24_DATARATE (RF24_1MBPS) // RF24_1MBPS for 1Mbps / RF24_2MBPS for 2Mbps // @note nRF24L01, BK2401, BK2421, BK2491 and XN297 does not support RF24_250KBPS
#define MY_RF24_PA_LEVEL (RF24_PA_MAX) // RF24 PA level for sending msgs // RF24_PA_MIN = -18dBm; RF24_PA_LOW = -12dBm; RF24_PA_HIGH = -6dBm; RF24_PA_MAX = 0dBm
#define MY_RF24_CHANNEL (85) //Set to 2485Mhz, to be outside of common used Wifi 2.4Ghz Bands & outside of medical device range
// https://en.wikipedia.org/wiki/List_of_WLAN_channels#2.4_GHz_(802.11b/g/n/ax)
// https://www.bipt.be/en/operators/radio/frequency-management/frequency-plan/table

#define CHILD_ID_TEMP 1
#define CHILD_ID_HUM 2
#define CHILD_ID_BARO 3
#define CHILD_ID_LCD 4
#define CHILD_ID_DFPLAYER 5

#define ONE_WIRE_BUS 4
#define TX_PIN 5 //Software serial for DFPlayer mini
#define RX_PIN 6 //Software serial for DFPlayer mini
#define SEND_ATTEMPTS 100

#include <MySensors.h>
#include <Wire.h>                     //i2c
#include <LiquidCrystal_I2C.h>        //http://downloads.arduino.cc/libraries/github.com/marcoschwartz/LiquidCrystal_I2C-1.1.2.zip
#include <OneWire.h>                  //Dallas OneWire protocol
#include <DallasTemperature.h>        //DS18b20
#include <BME280I2C.h>                //Bosh BME280 https://github.com/finitespace/BME280
#include <DFMiniMp3.h>                //DFPlayer mini https://github.com/Makuna/DFMiniMp3/wiki
#include <SoftwareSerial.h>           //Serial communication
#include <avr/wdt.h>

//Notification class with callbacks for DFPlayer Mini

typedef struct {
  float temperature;
  float humidity;
  float pressure;
} THPValues;

class Mp3Notify
{
public:
  static void OnError(uint16_t errorCode)
  {
  }
  static void OnPlayFinished(uint16_t globalTrack)
  {   
  }
  static void OnCardOnline(uint16_t code)
  {   
  }
  static void OnUsbOnline(uint16_t code)
  {   
  }
  static void OnCardInserted(uint16_t code)
  {
  }
  static void OnUsbInserted(uint16_t code)
  {  
  }
  static void OnCardRemoved(uint16_t code)
  {   
  }
  static void OnUsbRemoved(uint16_t code)
  {   
  }
};

MyMessage temperature_msg(CHILD_ID_TEMP, V_TEMP);
MyMessage humidity_msg(CHILD_ID_TEMP, V_HUM);
MyMessage baro_msg(CHILD_ID_BARO, V_PRESSURE);
MyMessage lcd_msg(CHILD_ID_LCD, V_TEXT);
/*
SoftwareSerial mySoftwareSerial(RX_PIN, TX_PIN);
DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(mySoftwareSerial);
*/
unsigned long latest_update_timestamp = 0;
unsigned long success_count = 0;
unsigned long fail_count = 0;
unsigned long fail_after_retries_count = 0;
unsigned long highest_nr_attempts = 0;
LiquidCrystal_I2C lcd(0x27,20,4);  // LCD i2c address 0x27, 20 columns & 4 rows
BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
float temp=NAN;
THPValues bmeReadings;

THPValues readBME280()
{
   THPValues response;
   float temp(NAN), hum(NAN), pres(NAN);
   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_hPa);
   bme.read(pres, temp, hum, tempUnit, presUnit);
   response.pressure = pres;
   response.temperature = temp;
   response.humidity = hum;   
   return (response);
}

void setup()
{
  /*
  mp3.begin(); mp3.setVolume(20);
  mp3.playMp3FolderTrack(2);
  */
  wdt_enable(WDTO_8S);
  lcd.init(); lcd.clear(); lcd.backlight();
  ds18b20.begin();  // Start up the Dallas library
  while(!bme.begin())
  {
    lcd.setCursor(0,0);
    lcd.print("No BME280 sensor!");
    delay(1000);
    wdt_reset();
  }
  lcd.clear();
}

void presentation()
{
  sendSketchInfo("Living room THP", "1.0");
  present(CHILD_ID_TEMP, S_TEMP);
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_BARO, S_BARO);
  present(CHILD_ID_LCD, S_INFO);
  present(CHILD_ID_DFPLAYER, S_CUSTOM);
}

bool sendMySensorsMsgWithRetries(MyMessage msg, int max_attempts){
  //return true if message was sent successfully, false otherwise.
  for (unsigned int cnt=1; cnt <= max_attempts; cnt++){
    if (cnt > highest_nr_attempts){
      highest_nr_attempts = cnt;
    }
    Serial.print("Sending message, attempt "); Serial.println(String(cnt));
      if (send(msg)){
        success_count +=1;
        return(true);
      }
      else{
        fail_count +=1;
        wait(50);
      }
    }
    fail_after_retries_count+=1;
    return (false);
}

void loop()
{
  wdt_reset();
  if ((millis() - latest_update_timestamp) > 30000){    
    
    latest_update_timestamp = millis();    
    ds18b20.requestTemperatures();    // sends command for all devices on the bus to perform a temperature conversion
    temp = ds18b20.getTempCByIndex(0); // get value from first DS18b20 in 1-wire network
    bmeReadings = readBME280();
    sendMySensorsMsgWithRetries(temperature_msg.set(temp, 1), SEND_ATTEMPTS);
    sendMySensorsMsgWithRetries(humidity_msg.set(bmeReadings.humidity, 0), SEND_ATTEMPTS);
    sendMySensorsMsgWithRetries(baro_msg.set(bmeReadings.pressure, 0), SEND_ATTEMPTS);
    lcd.setCursor(0,3); lcd.print("                    ");
  }
  lcd.setCursor(0,0); lcd.print("OK : "); lcd.print(String(success_count));
  lcd.setCursor(0,1); lcd.print("NOK: "); lcd.print(String(fail_count));lcd.print("->");lcd.print(String(fail_after_retries_count));
  lcd.setCursor(0,2); lcd.print("TOT: "); lcd.print(String(success_count+fail_count)); lcd.print(" - ");lcd.print(String(highest_nr_attempts));
  lcd.setCursor(0,3); lcd.print(String(temp, 1));lcd.print(" "); lcd.print(String(bmeReadings.humidity, 0));lcd.print(" "); lcd.print(String(bmeReadings.pressure, 0));
}
