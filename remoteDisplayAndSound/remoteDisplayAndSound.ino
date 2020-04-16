/*********************************************************************
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
 *  Nokia 5110 LCD display (connected via level shifter). Interface = SPI
 *  DFPlayer Mini - Connected via Serial Port (check PINs)
 *  Rotary encoder with push button (KY-040 Rotary Encoder) - Connected to Interrupt capable pins
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
 * SW (button)- 4
 * DT         - 3
 * CLK        - 2
 * 
 * Display connections
 * 1 VCC      - 3V3
 * 2 GND .    - GND
 * 3 SCE      - 5   - Via Level switcher
 * 4 RST      - 6   - Via Level switcher
 * 5 D/C      - 7   - Via Level switcher
 * 6 DN(MOSI) - 8   - Via Level switcher
 * 7 SCLK     - 9   - Via Level switcher
 * 8 LED      - 10  - Via Level switcher (10 supports PWM, so can control brightness)
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
 * DS18B20 Connections
 * VCC        - 3V3
 * GND        - GND
 * Data       - 18
 Arduino 2560 Pinout:
 
 Input and Output
 
 Each of the 54 digital pins on the Arduino 2560 Mega can be used as an input or output, using pinMode(), digitalWrite(), and digitalRead() functions.
 They operate at 5 volts. Each pin can provide or receive a maximum of 40 mA and has an internal pull-up resistor (disconnected by default) of 20-50 kOhms.


 * 
 * Variables   
 *  - BUITEN_TEMP
 *  - BUITEN_PRES
 *  - BUITEN_HUM
 *  
 *  - GLV_TEMP
 *  - GLV_PRES
 *  - GLV_HUM
 *  
 *  - SLKPN_TEMP
 *  - SLKPN_PRESS
 *  - SLKPN_HUM
 *  
 *  - SLKTN_TEMP
 *  - SLKTN_PRESS
 *  - SLKTN_HUM
 *  
 *  - KELDER_TEMP
 *  - KELDER_PRESS
 *  - KELDER_HUM
 *  
 *  - SOLAR_POWER
 *  - SOLAR_ENERGY
 *  
 *  - ZONSOPGANG
 *  - ZONSONDERGANG
 *  
 *  - VOORDEUR
 *  - ACHTERDEUR
 *  - POORT_BLB
 *  - POORT_PAM
 *  - SCHUIFRAAM
 *  - BERGING_RAAM
 *  - TOILET_RAAM
 *  - BUREAU_RAAM
 *  - RESET_DAY_MINMAX (aanroepen om min/max temp/vcochtigheid & luchtdruk aan te passen)
 *  
*********************************************************************/

#define LCD_SCLK D0
#define LCD_DIN  D5
#define LCD_DC   D6
#define LCD_RST  D7
#define LCD_CS   D8

#define ROTENC_CLK D2 //A0
#define ROTENC_DT  D3
#define ROTENC_SW  D4

#define DFPLAY_TX D6
#define DFPLAY_RX D5

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#include <DFMiniMp3.h>                //DFPlayer mini https://github.com/Makuna/DFMiniMp3/wiki
#include <SoftwareSerial.h>           //Serial communication - used for DFPlayer
 
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

Adafruit_PCD8544 display = Adafruit_PCD8544(LCD_SCLK, LCD_DIN, LCD_DC, LCD_CS, LCD_RST);
SoftwareSerial mySoftwareSerial(DFPLAY_RX, DFPLAY_TX);
DFMiniMp3<SoftwareSerial, Mp3Notify> dfplayer(mySoftwareSerial);

/**

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2


#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

void testdrawbitmap(const uint8_t *bitmap, uint8_t w, uint8_t h) {
  uint8_t icons[NUMFLAKES][3];
  randomSeed(666);     // whatever seed
 
  // initialize
  for (uint8_t f=0; f< NUMFLAKES; f++) {
    icons[f][XPOS] = random(display.width());
    icons[f][YPOS] = 0;
    icons[f][DELTAY] = random(5) + 1;
    
    Serial.print("x: ");
    Serial.print(icons[f][XPOS], DEC);
    Serial.print(" y: ");
    Serial.print(icons[f][YPOS], DEC);
    Serial.print(" dy: ");
    Serial.println(icons[f][DELTAY], DEC);
  }

  while (1) {
    // draw each icon
    for (uint8_t f=0; f< NUMFLAKES; f++) {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS], logo16_glcd_bmp, w, h, BLACK);
    }
    display.display();
    delay(200);

    while(Serial.available()) {
        switch (Serial.read()) {
          case 'w':display.setContrast(display.getContrast() + 1);
                   break;
          case 's':if(display.getContrast()) display.setContrast(display.getContrast() - 1);
                     break;
          case 'e':display.setBias(display.getBias() + 1);
                   break;
          case 'd':if(display.getBias()) display.setBias(display.getBias() - 1);
                   break;
          case 'R':display.setReinitInterval(10);
                   break;
          case 'r':display.initDisplay();
                   display.setReinitInterval(0);
                   break;
        }
    }
    Serial.print("contrast (w/s): 0x");
    Serial.println(display.getContrast(), HEX);
    Serial.print("   bias (e/d): 0x");
    Serial.println(display.getBias(), HEX);
    Serial.print("   reinitialize display (r/R): 0x");
    Serial.println(display.getReinitInterval(), HEX);

    // then erase it + move it
    for (uint8_t f=0; f< NUMFLAKES; f++) {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS],  logo16_glcd_bmp, w, h, WHITE);
      // move it
      icons[f][YPOS] += icons[f][DELTAY];
      // if its gone, reinit
      if (icons[f][YPOS] > display.height()) {
  icons[f][XPOS] = random(display.width());
  icons[f][YPOS] = 0;
  icons[f][DELTAY] = random(5) + 1;
      }
    }
   }
}


void testdrawchar(void) {
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);

  for (uint8_t i=0; i < 168; i++) {
    if (i == '\n') continue;
    display.write(i);
    //if ((i > 0) && (i % 14 == 0))
      //display.println();
  }    
  display.display();
}

void testdrawcircle(void) {
  for (int16_t i=0; i<display.height(); i+=2) {
    display.drawCircle(display.width()/2, display.height()/2, i, BLACK);
    display.display();
  }
}

void testfillrect(void) {
  uint8_t color = 1;
  for (int16_t i=0; i<display.height()/2; i+=3) {
    // alternate colors
    display.fillRect(i, i, display.width()-i*2, display.height()-i*2, color%2);
    display.display();
    color++;
  }
}

void testdrawtriangle(void) {
  for (int16_t i=0; i<min(display.width(),display.height())/2; i+=5) {
    display.drawTriangle(display.width()/2, display.height()/2-i,
                     display.width()/2-i, display.height()/2+i,
                     display.width()/2+i, display.height()/2+i, BLACK);
    display.display();
  }
}

void testfilltriangle(void) {
  uint8_t color = BLACK;
  for (int16_t i=min(display.width(),display.height())/2; i>0; i-=5) {
    display.fillTriangle(display.width()/2, display.height()/2-i,
                     display.width()/2-i, display.height()/2+i,
                     display.width()/2+i, display.height()/2+i, color);
    if (color == WHITE) color = BLACK;
    else color = WHITE;
    display.display();
  }
}

void testdrawroundrect(void) {
  for (int16_t i=0; i<display.height()/2-2; i+=2) {
    display.drawRoundRect(i, i, display.width()-2*i, display.height()-2*i, display.height()/4, BLACK);
    display.display();
  }
}

void testfillroundrect(void) {
  uint8_t color = BLACK;
  for (int16_t i=0; i<display.height()/2-2; i+=2) {
    display.fillRoundRect(i, i, display.width()-2*i, display.height()-2*i, display.height()/4, color);
    if (color == WHITE) color = BLACK;
    else color = WHITE;
    display.display();
  }
}
   
void testdrawrect(void) {
  for (int16_t i=0; i<display.height()/2; i+=2) {
    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, BLACK);
    display.display();
  }
}

void testdrawline() {  
  for (int16_t i=0; i<display.width(); i+=4) {
    display.drawLine(0, 0, i, display.height()-1, BLACK);
    display.display();
  }
  for (int16_t i=0; i<display.height(); i+=4) {
    display.drawLine(0, 0, display.width()-1, i, BLACK);
    display.display();
  }
  delay(250);
  
  display.clearDisplay();
  for (int16_t i=0; i<display.width(); i+=4) {
    display.drawLine(0, display.height()-1, i, 0, BLACK);
    display.display();
  }
  for (int8_t i=display.height()-1; i>=0; i-=4) {
    display.drawLine(0, display.height()-1, display.width()-1, i, BLACK);
    display.display();
  }
  delay(250);
  
  display.clearDisplay();
  for (int16_t i=display.width()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, i, 0, BLACK);
    display.display();
  }
  for (int16_t i=display.height()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, 0, i, BLACK);
    display.display();
  }
  delay(250);

  display.clearDisplay();
  for (int16_t i=0; i<display.height(); i+=4) {
    display.drawLine(display.width()-1, 0, 0, i, BLACK);
    display.display();
  }
  for (int16_t i=0; i<display.width(); i+=4) {
    display.drawLine(display.width()-1, 0, i, display.height()-1, BLACK); 
    display.display();
  }
  delay(250);
}
**/

void setup(){
  Serial.begin(115200);
  dfplayer.begin();
  uint16_t volume = dfplayer.getVolume();
  Serial.print("volume ");
  Serial.println(volume);
  dfplayer.setVolume(24);  
  uint16_t count = dfplayer.getTotalTrackCount(DfMp3_PlaySource_Sd);
  Serial.print("Aantal nummers: "); Serial.println(count);
  /*
  display.begin();
  // init done

  // you can change the contrast around to adapt the display
  // for the best viewing!
  display.setContrast(50);

  display.display(); // show splashscreen
  delay(2000);
  display.clearDisplay();   // clears the screen and buffer

  // draw a single pixel
  display.drawPixel(10, 10, BLACK);
  display.display();
  delay(2000);
  display.clearDisplay();

  // draw many lines
  testdrawline();
  display.display();
  delay(2000);
  display.clearDisplay();

  // draw rectangles
  testdrawrect();
  display.display();
  delay(2000);
  display.clearDisplay();

  // draw multiple rectangles
  testfillrect();
  display.display();
  delay(2000);
  display.clearDisplay();

  // draw mulitple circles
  testdrawcircle();
  display.display();
  delay(2000);
  display.clearDisplay();

  // draw a circle, 10 pixel radius
  display.fillCircle(display.width()/2, display.height()/2, 10, BLACK);
  display.display();
  delay(2000);
  display.clearDisplay();

  testdrawroundrect();
  delay(2000);
  display.clearDisplay();

  testfillroundrect();
  delay(2000);
  display.clearDisplay();

  testdrawtriangle();
  delay(2000);
  display.clearDisplay();
   
  testfilltriangle();
  delay(2000);
  display.clearDisplay();

  // draw the first ~12 characters in the font
  testdrawchar();
  display.display();
  delay(2000);
  display.clearDisplay();

  // text display tests
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
  delay(2000);

  // rotation example
  display.clearDisplay();
  display.setRotation(1);  // rotate 90 degrees counter clockwise, can also use values of 2 and 3 to go further.
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Rotation");
  display.setTextSize(2);
  display.println("Example!");
  display.display();
  delay(2000);

  // revert back to no rotation
  display.setRotation(0);

  // miniature bitmap display
  display.clearDisplay();
  display.drawBitmap(30, 16,  logo16_glcd_bmp, 16, 16, 1);
  display.display();

  // invert the display
  display.invertDisplay(true);
  delay(1000); 
  display.invertDisplay(false);
  delay(1000); 

  // draw a bitmap icon and 'animate' movement
  testdrawbitmap(logo16_glcd_bmp, LOGO16_GLCD_WIDTH, LOGO16_GLCD_HEIGHT);

  */
}

void loop()  {
  dfplayer.loop(); 
}
