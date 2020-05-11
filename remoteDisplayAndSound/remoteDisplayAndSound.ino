
 /*
 * Purpose
 * -------
 * This sketch implements a device that:
 * - Displays a set of variables (which can be set via a REST interface (using a Nokia 5110 display). A rotary encoder (KY-040) is used to navigate between different views
 * - Control an MP3 Player (DFPlayer mini) via a REST interface
 * 
 * For my setup, it will:
 * - Display outside & room temperature/humidity & barometric pressure
 * - Display solar power generation
 * - Display the status of the doors & gates
 * - Play a tune when a door/window is opened/closed
 * 
 * A domotics controller will control this node via a REST interface (based on aREST).
 * 
 * Hardware used & pinout
 * ----------------------
 * 
 *  Wemos D1 Mini Pro => Was not working with DFPlayer, even not with level shifter => Replaced with Arduino Mega 2560 + Ethernet board
 *  Nokia 5110 LCD display (connected via level shifter). Interface = SPI                         =====> FAILED FAILED
 *  DFPlayer Mini - Connected via Serial Port (check PINs)                                        =====> Connections OK, some jitter (missing resistor on TX/RX, capacitor between VCC & GND)?
 *  Rotary encoder with push button (KY-040 Rotary Encoder) - Connected to Interrupt capable pins =====> Connections OK
 *  |-- Rotary encoder push button ((KY-040 Rotary Encoder)                                       =====> TODO
 *  NRF24L01-PA-LNA wireless module                                                               =====> Connections OK 
 *  BME280 temperature, humidity & pressure sensor (I2C)                                          =====> Connections OK
 *  DS18B20 temperature sensor                                                                    =======> Still need to connect !!!!
 *  
 * * DFPlayer mini connections
 * VCC        - 5V
 * GND        - GND
 * TX         - 17 (RX2) 
 * RX         - 16 (TX2)
 * 
 * BME280 Connections
 * VCC        - 3V3
 * GND        - GND
 * SDA        - 20 (SDA) Level switcher
 * SCL        - 21 (SCL) Level switcher
 * 
 * Rotary encoder connections (int pins:  2, 3, 18, 19, 20, 21)
 * GND        - GND
 * +          - 5V
 * SW (button)- 42
 * DT         - 3
 * CLK        - 2
 * 
 * Display connections
 * 1 VCC      - 3V3
 * 2 GND .    - GND                                                                     LVL3V          LVL5V       NIEUW
 * 3 SCE      - 5   - Via Level switcher                                                Oranje B7      bruin A7     5     32
 * 4 RST      - 6   - Via Level switcher                                                Geel   B6      zwart A6     6     34
 * 5 D/C      - 7   - Via Level switcher                                                Groen  B5      wit   A5     7     36
 * 6 DN(MOSI) - 8   - Via Level switcher                                                Blauw  B4      grijs A4     8     38
 * 7 SCLK     - 9   - Via Level switcher                                                Paars  B3      paars A3     9     40
 * 8 LED      - 10  - Via Level switcher (10 supports PWM, so can control brightness)   Grijs  B2      blauw A2     10
 * 
 * NRF24L01-PA-LNA connections (using a power supply adapter board to provide stable 3.3V)
 * VCC        - 5V
 * GND        - GND
 * IRQ        - 19
 * MISO       - 22
 * MOSI       - 24
 * SCK        - 26
 * CSN        - 28
 * CE         - 30
 * 
 * DS18B20 Connections => Not connected yet
 * VCC        - 3V3
 * GND        - GND
 * Data       - 18

*********************************************************************/

#define ROTENC_CLK 2 //Interrupt capable
#define ROTENC_DT  3 //Interrupt capable
#define ROTENC_SW  42 //Button on rotary encoder

#define LCD_CS   32
#define LCD_RST  34
#define LCD_DC   36
#define LCD_DIN  38
#define LCD_SCLK 40
#define LCD_LED  10 //PWM

#define DFPLAY_RX 16 //Hardware TX2
#define DFPLAY_TX 17 //Hardware RX2

#define DS18B20_DATA 18

#define BME280_SDA 20
#define BME280_SCL 21

#define MY_MAC_ADDRESS 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x02

// MySensors part
#define MY_DEBUG
#define MY_RADIO_RF24
#define MY_REPEATER_FEATURE
//#define MY_RX_MESSAGE_BUFFER_FEATURE //does not work with software SPI
//#define MY_RF24_IRQ_PIN 19 //does not work with software SPI

#if !defined(MY_W5100_SPI_EN) && !defined(ARDUINO_ARCH_SAMD)
#define MY_SOFTSPI
#define MY_SOFT_SPI_MISO_PIN 22
#define MY_SOFT_SPI_MOSI_PIN 24
#define MY_SOFT_SPI_SCK_PIN 26
#endif

// When W5100 is connected we have to move CE/CSN pins for NRF radio
#ifndef MY_RF24_CS_PIN
#define MY_RF24_CS_PIN 28
#endif
#ifndef MY_RF24_CE_PIN
#define MY_RF24_CE_PIN 30
#endif

//Configure channel (frequency), transmission power & speed. Lower speed => higher range
#define MY_RF24_DATARATE (RF24_1MBPS) // RF24_1MBPS for 1Mbps / RF24_2MBPS for 2Mbps // @note nRF24L01, BK2401, BK2421, BK2491 and XN297 does not support RF24_250KBPS
#define MY_RF24_PA_LEVEL (RF24_PA_MAX) // RF24 PA level for sending msgs // RF24_PA_MIN = -18dBm; RF24_PA_LOW = -12dBm; RF24_PA_HIGH = -6dBm; RF24_PA_MAX = 0dBm
#define MY_RF24_CHANNEL (85) //Set to 2485Mhz, to be outside of common used Wifi 2.4Ghz Bands & outside of medical device range
// https://en.wikipedia.org/wiki/List_of_WLAN_channels#2.4_GHz_(802.11b/g/n/ax)
// https://www.bipt.be/en/operators/radio/frequency-management/frequency-plan/table

#define CHILD_ID_TEMP 1
#define CHILD_ID_HUM 2
#define CHILD_ID_BARO 3

#include <Ethernet.h>
#include <aREST.h>
#include <MySensors.h>
#include <SPI.h>
#include <Encoder.h>                  // Library for rotary encoders
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <DFMiniMp3.h>                //DFPlayer mini https://github.com/Makuna/DFMiniMp3/wiki
#include <Wire.h>                     //I2C
#include <BME280I2C.h>                //Bosh BME280 https://github.com/finitespace/BME280
#include <OneWire.h>                  //Dallas OneWire protocol
#include <DallasTemperature.h>        //DS18b20
#include <Button2.h>                  //https://github.com/LennartHennigs/Button2 Arduino library to simplify working with buttons

typedef struct {
  float temperature;
  float humidity;
  float pressure;
} THPValues;
 

//Callbacks for DFPlayer events
class Mp3Notify
{
  public:
    static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action)
    {
      if (source & DfMp3_PlaySources_Sd) 
      {
          Serial.print("SD Card, ");
      }
      if (source & DfMp3_PlaySources_Usb) 
      {
          Serial.print("USB Disk, ");
      }
      if (source & DfMp3_PlaySources_Flash) 
      {
          Serial.print("Flash, ");
      }
      Serial.println(action);
    }
    static void OnError(uint16_t errorCode)
    {
      // see DfMp3_Error for code meaning
      Serial.println();
      Serial.print("Com Error ");
      Serial.println(errorCode);
    }
    static void OnPlayFinished(DfMp3_PlaySources source, uint16_t track)
    {
      Serial.print("Play finished for #");
      Serial.println(track);
    }
    static void OnPlaySourceOnline(DfMp3_PlaySources source)
    {
      PrintlnSourceAction(source, "online");
    }
    static void OnPlaySourceInserted(DfMp3_PlaySources source)
    {
      PrintlnSourceAction(source, "inserted");
    }
    static void OnPlaySourceRemoved(DfMp3_PlaySources source)
    {
      PrintlnSourceAction(source, "removed");
    }
};

EthernetServer server = EthernetServer(80); //Ethernet server listening on TCP port 80
aREST rest = aREST();
Adafruit_PCD8544 display = Adafruit_PCD8544(LCD_SCLK, LCD_DIN, LCD_DC, LCD_CS, LCD_RST);
DFMiniMp3<HardwareSerial, Mp3Notify> dfplayer(Serial2);
Encoder rotaryEncoder(ROTENC_CLK, ROTENC_DT);
long oldPosition  = -999;
BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
                  // Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,
Button2 button = Button2(ROTENC_SW);

//Messages to send over the mySensors network
MyMessage temperature_msg(CHILD_ID_TEMP, V_TEMP);
MyMessage humidity_msg(CHILD_ID_TEMP, V_HUM);
MyMessage baro_msg(CHILD_ID_BARO, V_PRESSURE);
unsigned long latest_update_timestamp = 0;

//variables exposed to aREST
volatile unsigned long switch_voordeur = 0;
volatile unsigned long switch_achterdeur = 0;
volatile unsigned long switch_schuifraam = 0;
volatile unsigned long switch_bergingraam = 0;
volatile unsigned long switch_toiletraam = 0;
volatile unsigned long switch_bureauraam = 0;
volatile unsigned long switch_poortpamel = 0;
volatile unsigned long switch_poortblb = 0;
volatile unsigned long temp_outside = 0;
volatile unsigned long hum_outside = 0;
volatile unsigned long temp_slaapkamer_master = 0;
volatile unsigned long hum_slaapkamer_master = 0;
volatile unsigned long temp_kelder = 0;
volatile unsigned long hum_kelder = 0;
volatile unsigned long temp_kamer_thijs = 0;
volatile unsigned long hum_kamer_thijs = 0;
volatile unsigned long temp_kamer_niels = 0;
volatile unsigned long hum_kamer_niels = 0;
volatile unsigned long solar_kwhday = 0;
volatile unsigned long solar_power = 0;

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

void waitMilliseconds(uint16_t msWait)
{
  uint32_t start = millis();
  
  while ((millis() - start) < msWait)
  {
    // calling mp3.loop() periodically allows for notifications 
    // to be handled without interrupts
    dfplayer.loop(); 
    delay(1);
  }
}

//Event handlers for button
void pressed(Button2& btn) {
    Serial.println("pressed");
}
void released(Button2& btn) {
    Serial.print("released: ");
    Serial.println(btn.wasPressedFor());
}
void changed(Button2& btn) {
    Serial.println("changed");
}
void click(Button2& btn) {
    Serial.println("click\n");
}
void longClick(Button2& btn) {
    Serial.println("long click\n");
}
void doubleClick(Button2& btn) {
    Serial.println("double click\n");
}
void tripleClick(Button2& btn) {
    Serial.println("triple click\n");
}
void tap(Button2& btn) {
    Serial.println("tap");
}

void display_something(){ 
  display.setContrast(60);  
  display.clearDisplay();   // clears the screen and buffer
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Hello, world!");
  display.setTextColor(WHITE, BLACK); // 'inverted' text
  display.println(3.141592);
  display.setTextSize(2);
  display.setTextColor(BLACK);
  display.print("0x"); display.println(0xDEADBEEF, HEX);
  display.display();
}

int playSound(String command) {  
  int tracknumber = 0;
  tracknumber = command.toInt();
  dfplayer.playMp3FolderTrack(tracknumber);
  return 0;
}

int changeVolume(String command) {  
  int volume = 0;
  volume = command.toInt();
  if ((volume >=0) and (volume <=32)){
    dfplayer.setVolume(volume);
  }
  return 0;
}

void setup(){
  Serial.begin(115200);

  byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
  };
  
  if (Ethernet.begin(mac) == 0) {
  Serial.println("Failed to configure Ethernet using DHCP");
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } 
    else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
  }
  // print your local IP address:
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
  
  // REST interface: Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("111000");
  rest.set_name((char*)"remoteDisplaySound");
  
  rest.function((char*)"playSound", playSound); // register function to REST interface
  rest.function((char*)"changeVolume", changeVolume);
  
  //Door & Window switches
  rest.variable("switch_voordeur",&switch_voordeur);
  rest.variable("switch_achterdeur",&switch_achterdeur);
  rest.variable("switch_schuifraam",&switch_schuifraam);
  rest.variable("switch_bergingraam",&switch_bergingraam);
  rest.variable("switch_toiletraam",&switch_toiletraam);
  rest.variable("switch_bureauraam",&switch_bureauraam);
  rest.variable("switch_poortpamel",&switch_poortpamel);
  rest.variable("switch_poortblb",&switch_poortblb);
  
  //Temperature & humidity sensors
  rest.variable("temp_outside",&temp_outside);
  rest.variable("hum_outside",&hum_outside);  
  rest.variable("temp_slaapkamer_master",&temp_slaapkamer_master);  
  rest.variable("hum_slaapkamer_master",&hum_slaapkamer_master);  
  rest.variable("temp_kelder",&temp_kelder);
  rest.variable("hum_kelder",&hum_kelder);
  rest.variable("temp_kamer_thijs",&temp_kamer_thijs);  
  rest.variable("hum_kamer_thijs",&hum_kamer_thijs);
  rest.variable("temp_kamer_niels",&temp_kamer_niels);  
  rest.variable("hum_kamer_niels",&hum_kamer_niels);  
  rest.variable("solar_kwhday",&solar_kwhday);
  rest.variable("solar_power",&solar_power);
  
  //Start ethernet server
  server.begin();
  
  pinMode(LCD_LED, OUTPUT);  // sets the pin as output  
  analogWrite(LCD_LED,255);  // Enable backlight, full power

  //Set callbacks for the push button
  button.setChangedHandler(changed);
  button.setPressedHandler(pressed);
  button.setReleasedHandler(released);
  button.setTapHandler(tap);
  button.setClickHandler(click);
  button.setLongClickHandler(longClick);
  button.setDoubleClickHandler(doubleClick);
  button.setTripleClickHandler(tripleClick);
  
  dfplayer.begin();
  display_something();
  
  uint16_t volume = dfplayer.getVolume();
  Serial.print("volume ");
  Serial.println(volume);
  dfplayer.setVolume(18);  
  dfplayer.playMp3FolderTrack(1);

  //I2C initialisation  
  Wire.begin();
  while(!bme.begin())
  {
    Serial.println("Failed to initialise BME280");
    delay(1000);
  }
}

void presentation()
{
  //Send the sensor node sketch version information to the gateway
  sendSketchInfo("RemoteDisplay", "1.0");
  present(CHILD_ID_TEMP, S_TEMP);
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_BARO, S_BARO);
}

void loop()  {
  dfplayer.loop();
  button.loop();
  long newPosition = rotaryEncoder.read();
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    Serial.println(newPosition);
  }
  if ((millis() - latest_update_timestamp) > 5000){
    latest_update_timestamp = millis();  
    Serial.println("Reading BME280 values");
    bmeReadings = readBME280();
    Serial.print("BME280 Temperature: "); Serial.println(bmeReadings.temperature);
    Serial.print("BME280 Humidity: "); Serial.println(bmeReadings.humidity);
    Serial.print("BME280 Pressure: "); Serial.println(bmeReadings.pressure);
  }
}
