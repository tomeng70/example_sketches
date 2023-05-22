#include <TimeLib.h>
#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

// When using the BREAKOUT BOARD only, use these 8 data lines to the LCD:
// For the Arduino Uno, Duemilanove, Diecimila, etc.:
//   D0 connects to digital pin 8  (Notice these are
//   D1 connects to digital pin 9   NOT in order!)
//   D2 connects to digital pin 2
//   D3 connects to digital pin 3
//   D4 connects to digital pin 4
//   D5 connects to digital pin 5
//   D6 connects to digital pin 6
//   D7 connects to digital pin 7
// For the Arduino Mega, use digital pins 22 through 29
// (on the 2-row header at the end of the board).

// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// More color definitions
#define ILI9341_BLACK       0x0000      /*   0,   0,   0 */
#define ILI9341_NAVY        0x000F      /*   0,   0, 128 */
#define ILI9341_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define ILI9341_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define ILI9341_MAROON      0x7800      /* 128,   0,   0 */
#define ILI9341_PURPLE      0x780F      /* 128,   0, 128 */
#define ILI9341_OLIVE       0x7BE0      /* 128, 128,   0 */
#define ILI9341_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define ILI9341_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define ILI9341_BLUE        0x001F      /*   0,   0, 255 */
#define ILI9341_GREEN       0x07E0      /*   0, 255,   0 */
#define ILI9341_CYAN        0x07FF      /*   0, 255, 255 */
#define ILI9341_RED         0xF800      /* 255,   0,   0 */
#define ILI9341_MAGENTA     0xF81F      /* 255,   0, 255 */
#define ILI9341_YELLOW      0xFFE0      /* 255, 255,   0 */
#define IL9I341_WHITE       0xFFFF      /* 255, 255, 255 */
#define ILI9341_ORANGE      0xFD20      /* 255, 165,   0 */
#define ILI9341_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define ILI9341_PINK        0xF81F


#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

// the following min and max values
// for the coordinate axes of the touch screen
// were obtained by displaying the x and y values 
// for my screen when the screen was being pressed with 
// a touchscreen stylus. (T. Eng, 5/19/2023)
#define TS_MINX 105
#define TS_MAXX 940
#define TS_MINY 73
#define TS_MAXY 922

// We have a status line for like, is FONA working
#define STATUS_X 10
#define STATUS_Y 65


class TouchArea {
 public:
  // constructor
  TouchArea(void)
  {
     _gfx = 0;
  }

  // initialize the touch area.
  void initArea(Elegoo_GFX *gfx, int16_t x, int16_t y, uint8_t w, uint8_t h, uint16_t outline) { 
    _x = x;
    _y = y;
    _w = w;
    _h = h;
    _outlinecolor = outline;
    _gfx = gfx;
  }

  // draw the outline of the touch area.
  void drawArea(bool eraseFlag=false) {
    uint16_t outline;
    if (eraseFlag == false) {
      outline = _outlinecolor;
    } else {
      outline = BLACK;
    }
    
    _gfx->drawRect(_x, _y, _w, _h, outline);   
  }

  boolean contains(int16_t x, int16_t y) {
   if ((x < (_x - _w/2)) || (x > (_x + _w/2))) return false;
   if ((y < (_y - _h/2)) || (y > (_y + _h/2))) return false;
   return true;
  }

  void press(boolean p) {
    laststate = currstate;
    currstate = p;
  }

  boolean isPressed() {
    return currstate;
  }

  boolean justPressed() {
    return (currstate && !laststate);
  }

  boolean justReleased() {
    return (!currstate && laststate);
  }

 private:
  Elegoo_GFX *_gfx;
  int16_t _x, _y;
  uint16_t _w, _h;
  uint16_t _outlinecolor;

  boolean currstate, laststate;
};

// Create a TFT object that we can use to interat with the touch screen.
Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

// Create a touch screen object.
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// enum to keep track of the state of the system.
enum State {
  DISPLAY_TIME,
  SET_HOUR,
  SET_MINUTE,
  SET_SECOND
};

// keep track of the current state.
State currState;

// time related global variables.
char bufTime[9];
time_t currTime;
long h;
long m;
long s;

// track elapsed time to decide when to refresh display.
#define DISPLAY_REFRESH_MSEC 500
unsigned long prevDisplayTime;
unsigned long currDisplayTime;

// set time button.
Elegoo_GFX_Button btnSet;

// touch areas to set time.
TouchArea touchHour;
TouchArea touchMinute;
TouchArea touchSecond;

void initSystem() {
  // initialize the system.
  currState = DISPLAY_TIME;

  // set previous time to 0.
  prevDisplayTime = 0;
}

void initButtons() {
  // initialize set button
  btnSet.initButton(&tft,100, 180, 80, 40, IL9I341_WHITE, ILI9341_CYAN, ILI9341_BLUE, "Set", 2);
  btnSet.drawButton(false);
}

void initTouchAreas() {
  touchHour.initArea(&tft, 2, 0, 50, 40, YELLOW);
  touchMinute.initArea(&tft, 74,0, 50, 40, YELLOW);
  touchSecond.initArea(&tft, 146,0, 50, 40, YELLOW);
}

void drawButtons() {
  // check to see if just pressed or just released.
  if (btnSet.justPressed()) {
    // invert colors to indicate button is currently pressed.
    //btnSet.drawButton(true);
    Serial.println("*** Test was just pressed! ***");
  } else if(btnSet.justReleased()) {
    // return to normal colors to indicate that the button is currently unpressed.
    Serial.println("*** Test just released. ***");
    //btnSet.drawButton(false);
  } 
    
  // let user know the button is pressed through serial connection
  if(btnSet.isPressed()) {
    Serial.println("test is pressed!");
  }
}

// check what the state of the buttons are.
void checkButtons() {
  // switch to touchscreen
  digitalWrite(13, HIGH);

  // get the current touch screen point.
  TSPoint p = ts.getPoint();

  // when the display is in portrait mode (tft.setRotation(1))
  // it appears that the display x axis corresponds to the touch screen y axis
  // and the display y axis corresponds to the touch screen x axis.
  // also it appears that the touch screen and display axes point in opposite directions.
  // create two variables, xpos and ypos, that will continue the x and y coordinates in terms of the display axes.
  // use the map function to map from the touch screen coordinates to the display coordinates.
  int16_t xpos, ypos;  //screen coordinates
  xpos = map(p.y, TS_MAXY, TS_MINY,  0, tft.width());
  ypos = map(p.x, TS_MAXX, TS_MINX, 0, tft.height());

  // switch back to display mode.
  digitalWrite(13, LOW);
  // if sharing pins, you'll need to fix the directions of the touchscreen pins
  //pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  //pinMode(YM, OUTPUT);

  // check to see if the user is pressing in the area of the button.
  if(p.z > 0 && btnSet.contains(xpos, ypos)) {
    // i think this is when user first presses screen.
    btnSet.press(true);
  } else if (btnSet.isPressed() && btnSet.contains(xpos, ypos)) {
    // i think this is when user has kept finger still pressed against screen.
    btnSet.press(true);
  } else {
    btnSet.press(false);
  }
}

// check to see if we need to change the system state.
void checkSysState() {
  // was the set button just pressed?
  if (btnSet.justPressed()) {
    if (currState == DISPLAY_TIME) {
      // switch to set hour mode.
      currState = SET_HOUR;
      Serial.println("SET_HOUR mode");
      btnSet.drawButton(true);
    } else if (currState == SET_HOUR) {
      // switch to set minute mode.
      currState = SET_MINUTE; 
      Serial.println("SET_MINUTE mode");
      btnSet.drawButton(true);
    } else if (currState == SET_MINUTE) {
      currState = SET_SECOND;
      Serial.println("SET_SECOND mode");
      //btnSet.drawButton(true);
    } else {
      // by default switch back to DISPLAY_TIME
      currState = DISPLAY_TIME;
      Serial.println("DISPLAY_TIME mode");
      btnSet.drawButton(false);
    }
  }
}

void initScreen() {
  // which pinout configuration to use?
#ifdef USE_Elegoo_SHIELD_PINOUT
  Serial.println(F("Using Elegoo 2.4\" TFT Arduino Shield Pinout"));
#else
  Serial.println(F("Using Elegoo 2.4\" TFT Breakout Board Pinout"));
#endif

  // print display screen dimensions (in pixels).
  Serial.print("TFT size is "); Serial.print(tft.width()); Serial.print("x"); Serial.println(tft.height());

  // reset screen.
  tft.reset();

  // determine what LCD chip is used by the screen.
   uint16_t identifier = tft.readID();
   if(identifier == 0x9325) {
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if(identifier == 0x9328) {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if(identifier == 0x4535) {
    Serial.println(F("Found LGDP4535 LCD driver"));
  }else if(identifier == 0x7575) {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if(identifier == 0x9341) {
    Serial.println(F("Found ILI9341 LCD driver"));
  } else if(identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
  } else if(identifier==0x0101)
  {     
      identifier=0x9341;
       Serial.println(F("Found 0x9341 LCD driver"));
  }
  else if(identifier==0x1111)
  {     
      identifier=0x9328;
       Serial.println(F("Found 0x9328 LCD driver"));
  }
  else {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.println(F("If using the Elegoo 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_Elegoo_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Elegoo_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
    identifier=0x9328;
  }

  // start the screen.
  tft.begin(identifier);

  // set to landscape mode.
  tft.setRotation(1);

  // clear screen (to color black).
  tft.fillScreen(BLACK);
} 

void updateCurrTime() {
  // use the now() function to get the time all at once (so the seconds, minutes, and hours will be consistent).
  currTime = now();
  h = hour(currTime);
  m = minute(currTime);
  s = second(currTime);
  
  // use sprintf to format the output into the character buffer.
  // note that the sprintf %d formatting tags expect ints and not longs, so cast values to int type.
  sprintf(bufTime, "%02d:%02d:%02d", (int)h, (int)m, (int)s);

  // debug display character buffer in serial output.
  //Serial.println(bufTime);
}

// render the time on the screen.
void renderCurrTime() {
  // erase the old time.
    tft.fillRect(0, 0, 205, 45, BLACK);

    // write the time on the display.
    tft.setCursor(5, 5);  
    tft.setTextColor(BLUE);  
    tft.setTextSize(4);
    tft.println(bufTime);
}

// the function displayCurrTime() checks to see if a certain amount of time
// has passed before it will refresh the time on the display.
// this is done to reduce the flicker of the screen.
void displayCurrTime() {
  // get the current time.
  currDisplayTime = millis();

  // has enough time passed so we can refresh screen?
  if (currDisplayTime - prevDisplayTime > DISPLAY_REFRESH_MSEC) {
    // render the current time.
    renderCurrTime();
    
    // since we refreshed the display, the current time becomes the previous time.
    prevDisplayTime = currDisplayTime;
  }  
}

void setTime() {
  // highlight the area to be set and check to see if screen is pressed.
  if (currState == SET_HOUR) {
    // draw a rectangle around the hour field.
    touchHour.drawArea();
  } else if (currState == SET_MINUTE) {
    // erase previous rectangle (from set hour)
    touchHour.drawArea(true);

    // draw a rectangle around the minute field.
    touchMinute.drawArea();
  } else if (currState == SET_SECOND) {
    // erase previous rectangle (from set hour)
    touchMinute.drawArea(true);

    // draw a rectangle around the second field.
    touchSecond.drawArea();
  }
}

void setup() {
  // initialize serial connection for debugging.
  Serial.begin(9600);

  // populate the character buffer with null characters.
  int i;
  for (i = 0; i < 9; i++) {
    bufTime[i] = NULL;
  }

  // initialize screen.
  initScreen();

  // initialize buttons.
  initButtons();

  // initialize touch areas.
  initTouchAreas();

  // initialize system.
  initSystem();
}

void loop() {
  // check buttons.
  checkButtons();

// we can draw set button when we check system state.

    // draw buttons.
  //drawButtons();
  

  // check the system state.
  checkSysState();

  // what state are we in?
  if (currState == DISPLAY_TIME) {
    // update time.
    updateCurrTime();

    // display time.
    displayCurrTime();
  } else if (currState == SET_HOUR 
    || currState == SET_MINUTE 
    || currState == SET_SECOND) {
    // set time.
    setTime();
  } 
    
  delay(100);
}
