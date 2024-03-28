/**************************************************************************
 MIDI 2 CV Display, buttons and DACs test
 HW 5.1
 **************************************************************************/

// DEBUG flag disables MIDI and activates Serial
//#define DEBUG

//#ifdef DEBUG
  #include <MemoryUsage.h>
//#endif


// MIDI *************************************
#ifndef DEBUG
  #include <MIDI.h>

  #define MIDI_IN 0
  #define MIDI_LED A3

  // MIDI Channel we want to react to
  #define MIDI_CHANNEL MIDI_CHANNEL_OMNI

  MIDI_CREATE_DEFAULT_INSTANCE();
#endif

void toggle_MIDI_LED(void)
{
  // pin PC3 is connected to MIDI activity LED
  PORTC ^= (1 << 3);
}


// screen *************************************
/*
  OLED Displays only require 1 byte for every 8 pixels
  The 128x64 version requires 1K of SRAM
*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0xBC ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

/*
 * BC =    1011 1100
 * 78 =    0111 1000
 * 178 = 1 0111 1000
 * 3C =    0011 1100
 * 
 * See https://community.element14.com/members-area/personalblogs/b/blog/posts/oled-i2c-silkscreens-are-wrong-or-are-they
 */
 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



// Keys *************************************
#include <KeyDetector.h>

// Pins the buttons are connected to
//const byte upPin = A0;
//const byte enterPin = A1;
//const byte downPin = A2;
#define UPPIN A0
#define ENTERPIN A1
#define DOWNPIN A2

#define KEY_UP 1
#define KEY_ENTER 2
#define KEY_DOWN 3

Key keys[] = {{KEY_UP, UPPIN}, {KEY_ENTER, ENTERPIN}, {KEY_DOWN, DOWNPIN}};

KeyDetector myKeyDetector(keys, sizeof(keys)/sizeof(Key), 10, 0, true);
// true is for PULLUP



// LEDs *************************************
//const int ledPin1 =  2;      // the number of the LED pin
//const int ledPin2 =  3;      // the number of the LED pin
//const int ledPin3 =  4;      // the number of the LED pin
#define LEDPIN1 2
#define LEDPIN2 3
#define LEDPIN3 4



// DACs ************************************
#include <MCP48xx.h>

// Define the MCP4822 instance, giving it the CS (Chip Select) pin
// The constructor will also initialize the SPI library
// We can also define a MCP4812 or MCP4802
MCP4822 dac1(9);
MCP4822 dac2(10);

// We define an int variable to store the voltage in mV so 100mV = 0.1V
int cv[4] = {0};




// menu ************************************
bool menu_mode = false;     // menu_mode is true when a key has been pressed, then set back to false after key_timeout
long last_key_press = 0;    // last time a key was pressed
//const long key_timeout = 3000;    // timeout for key press, before exiting menu mode (in ms)
#define KEY_TIMEOUT 3000    // timeout for key press, before exiting menu mode (in ms)
byte current_param = 0;     // parameter being edited

struct struct_param{
   char Name[8];
   int minval;
   int maxval;
//   int value;
};

#define N_PARAMS 2
//const struct_param params[N_PARAMS] = {
//  "MIDI Ch", 0, 16, 0,
//  "Mode   ", 1, 6,  1
//};
const struct_param params[N_PARAMS] PROGMEM = {
  "Mode   ", 1, 6,
  "MIDI Ch", 0, 16,
};
// separate array for non const/PROGMEM values
int param_vals[N_PARAMS] = {
  1,
  0
};

byte selected_cv = 0;



void setup()
{
  pinMode(UPPIN,    INPUT_PULLUP);
  pinMode(ENTERPIN, INPUT_PULLUP);
  pinMode(DOWNPIN,  INPUT_PULLUP);

  // initialize the LED pin as an output:
  pinMode(LEDPIN1, OUTPUT);
  pinMode(LEDPIN2, OUTPUT);
  pinMode(LEDPIN3, OUTPUT);

  #ifdef DEBUG
    Serial.begin(115200);
    FREERAM_PRINT;
  #endif

  dac1.init();
  dac2.init();

  // The channels are turned off at startup so we need to turn the channel we need on
  dac1.turnOnChannelA();
  dac1.turnOnChannelB();
  dac2.turnOnChannelA();
  dac2.turnOnChannelB();

  // We configure the channels in High gain
  // It is also the default value so it is not really needed
  dac1.setGainA(MCP4822::High);
  dac1.setGainB(MCP4822::High);
  dac2.setGainA(MCP4822::High);
  dac2.setGainB(MCP4822::High);

  dac1.setVoltageA(cv[0]);
  dac1.setVoltageB(cv[1]);
  dac2.setVoltageA(cv[2]);
  dac2.setVoltageB(cv[3]);

  // We send the command to the MCP4822
  // This is needed every time we make any change
  dac1.updateDAC();
  dac2.updateDAC();
  
  
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    #ifdef DEBUG
      Serial.println(F("SSD1306 allocation failed"));
    #endif
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK); // Draw white text
  //display.setCursor(0, 0);     // Start at top-left corner
  //display.write("Hello");
  //display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);

  //display.write(F("0123456789a"));
  display.display();          // update display
  //for(;;); // Don't proceed, loop forever
  //displayCV();

  #ifndef DEBUG
    // Connect the handleNoteOn function to the library,
    // so it is called upon reception of a NoteOn.
    MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function
  
    // Do the same for NoteOffs
    MIDI.setHandleNoteOff(handleNoteOff);
  
    // Initiate MIDI communications, listen to channel 10 only, for drums
    MIDI.begin(MIDI_CHANNEL);
  #else
    FREERAM_PRINT;
  #endif
}

void show_menu(struct_param param);

void show_menu(struct_param param)
{
/*  #ifdef DEBUG
    Serial.print(F("param: "));
    Serial.print(params[current_param].Name);
    Serial.print(F(" = "));
    Serial.println(params[current_param].value);
  #endif

  display.clearDisplay();
  display.setCursor(0, 0);     // Start at top-left corner
  display.println(F("Options"));
  display.print(params[current_param].Name);
  display.print(F(" "));
  //display.println(params[current_param].value);
  display.println(param_vals[current_param]);
*/
  #ifdef DEBUG
    Serial.print(F("param: "));
    Serial.print(param.Name);
    Serial.print(F(" = "));
    Serial.println(param_vals[current_param]);
    FREERAM_PRINT;
  #endif

  display.clearDisplay();
  display.setCursor(0, 0);     // Start at top-left corner
  display.println(F("Options"));
  display.print(param.Name);
  display.print(F(" "));
  //display.println(params[current_param].value);
  display.println(param_vals[current_param]);

  display.print(mu_freeRam());
  
  display.display();          // update display
}

void hide_menu(void)
{
  display.clearDisplay();
  display.setCursor(0, 0);     // Start at top-left corner

  display.print(mu_freeRam());

  display.display();          // update display
  #ifdef DEBUG
    FREERAM_PRINT;
  #else
    display.print(mu_freeRam());
  #endif
}

void manage_menu(void)
{
  myKeyDetector.detect();

  if ((menu_mode == true) && millis() > (last_key_press + KEY_TIMEOUT))
  {
    menu_mode = false;
    #ifdef DEBUG
      Serial.println(F("timeout"));
    #endif
    // hide that menu
    hide_menu();
  }
  
  if (myKeyDetector.trigger)
  {
    struct_param param;
    memcpy_P(&param, &params[current_param], sizeof( struct_param));
    
    if (menu_mode == false)
    {
      // not yet in menu_mode
      menu_mode = true;     // menu_mode is true when a key has been pressed
    }
    else
    {
      switch (myKeyDetector.trigger)
      {
        case KEY_UP:
          // Up button was pressed
          param_vals[current_param]++;
          if (param_vals[current_param] > param.maxval)
          {
            param_vals[current_param] = param.maxval;
          }

          break;
        case KEY_ENTER:
          // Enter button was pressed
          current_param++;
          if (current_param > N_PARAMS - 1)
          {
            current_param = 0;
          }

          memcpy_P(&param, &params[current_param], sizeof( struct_param));

          break;
        case KEY_DOWN:
          // Down button was pressed
          param_vals[current_param]--;
          if (param_vals[current_param] < param.minval)
          {
            param_vals[current_param] = param.minval;
          }

          break;
        case KEY_NONE:
          //NO button was pressed
          //digitalWrite(LEDPIN1, LOW);
          //digitalWrite(LEDPIN2, LOW);
          //digitalWrite(LEDPIN3, LOW);
          break;
      }
    }

    show_menu(param);
    last_key_press = millis();      // save the last time a key was pressed
  }
}

void loop()
{
  #ifndef DEBUG
    // Call MIDI.read the fastest you can for real-time performance.
    MIDI.read();
  #endif

  manage_menu();
}
/*
void displayCV()
{
  display.setCursor(0, 0);     // Start at top-left corner

  // iterate for each DAC output
  for (byte i = 0; i < 4; i++)
  {
    // print a cursor for selected CV
    if (i == selected_cv)
    {
      display.print(F("> "));  // print the cursor
    }
    else
    {
      display.print(F("  "));  // erase the cursor
    }
    
    display.print(cv[i]);     // print the value (in mV)
    display.println(F("   "));   // erase trailing characters and go to next line
  }
  
  display.display();          // update display
}
*/

#define MODE_PARAM 0                            // the index where the "mode" is defined in array param_vals[]
#define CURRENT_MODE  param_vals[MODE_PARAM]    // macro to retrieve the current mode from  array param_vals[]

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  toggle_MIDI_LED();

  //depending on mode, do...
  switch (CURRENT_MODE)
  {
    case 1:
      display.print(F("ON"));
      //set dac 1 to pitch
      //set dac 2 to velocity
      //gate 1 ON
      break;
    case 2:
      break;
  }

  toggle_MIDI_LED();
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  toggle_MIDI_LED();

  //depending on mode, do...
  switch (CURRENT_MODE)
  {
    case 1:
      display.print(F("OFF"));
      //if queue empty
      //  gate 1 OFF

      // VOIR EXEMPLE CVPAL
      
      break;
    case 2:
      break;
  }

  toggle_MIDI_LED();
}
