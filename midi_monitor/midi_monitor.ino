/**************************************************************************
 MIDI 2 CV
 HW 0.5.1

 Monitor MIDI IN messages, scroll text with display buffer manipulation
 
 **************************************************************************/

#define DEBUG

#ifdef DEBUG
  #include <MemoryUsage.h>
#endif

#include <MIDI.h>

// Pin definitions
#define MIDI_LED A3

#define MIDI_IN 0

// MIDI Channel we want to react to
#define MIDI_CHANNEL MIDI_CHANNEL_OMNI

MIDI_CREATE_DEFAULT_INSTANCE();

void toggle_MIDI_LED(void)
{
  // pin PC3/A3 is connected to MIDI activity LED
  PORTC ^= (1 << 3);
}

// max queue allowed for display: when Serial queue is > MAX_DISP_QUEUE then, message isn't displayed, in order to speed up treatment of incoming MIDI messages
#define MAX_DISP_QUEUE 10


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


#include "scrolltext.h"

// define a scroll area starting on line 3, ending on line 8
ScrollText st(&display, 2, 7);

void setup()
{

  // *** MIDI ***
  // initialize the MIDI in pin as input
  pinMode(MIDI_IN, INPUT);

  // initialize the MIDI out pin as output
  pinMode (MIDI_LED, OUTPUT);

  // initialize MIDI LED state (off) and blink
  for (uint8_t i = 0; i < 4; i++)
  {
    digitalWrite(MIDI_LED, LOW);
    delay(50);
    digitalWrite(MIDI_LED, HIGH);
    delay(100);
  }


  // Connect the handleNoteOn function to the library,
  // so it is called upon reception of a NoteOn.
  MIDI.setHandleNoteOn(handleNoteOn);

  // Do the same for NoteOffs
  MIDI.setHandleNoteOff(handleNoteOff);

  MIDI.setHandlePitchBend(handlePitchBend);
  MIDI.setHandleControlChange(handleControlChange);
  MIDI.setHandleProgramChange(handleProgramChange);

  MIDI.setHandleClock(handleClock);
  MIDI.setHandleTuneRequest(handleTuneRequest);
  MIDI.setHandleStart(handleStart);
  MIDI.setHandleContinue(handleContinue);
  MIDI.setHandleStop(handleStop);

  MIDI.setHandleActiveSensing(handleActiveSensing);
  MIDI.setHandleSystemReset(handleSystemReset);

  // Initiate MIDI communications
  MIDI.begin(MIDI_CHANNEL);

  // *** Do NOT activate HW Serial as it interacts with MIDI communication ***


  
  // *** OLED display initialization ***
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    for(;;); // Don't proceed, loop forever
  }


  display.clearDisplay();                             // Clear the buffer
  display.setTextSize(1);                             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK); // Draw white text

  display.println(F("MIDI Monitor"));                 // print welcome message
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  st.initCursor();            // set to 1st line in scroll zone

  display.println(F("Ver. 20240327"));
  
  #ifdef DEBUG
    display.print(F("Free: "));
    display.println(mu_freeRam());
  #endif

  display.display();          // update display
}


void loop()
{
  // Call MIDI.read the fastest you can for real-time performance.
  MIDI.read();
}


void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  // Do something when the note is pressed.

  toggle_MIDI_LED();

  byte in_queue = Serial.available();
  
  // display only if queue isn't too long
  if (in_queue < MAX_DISP_QUEUE)
  {
    st.print(F("ON  "));
    char note_on_str[9] = "";
    snprintf(note_on_str, 9, "%02X %02X %02X", channel, pitch, velocity);
    st.println(note_on_str);

    st.show();
  }

  toggle_MIDI_LED();
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  // Do something when the note is released.
  // Note that NoteOn messages with 0 velocity are interpreted as NoteOffs.

  toggle_MIDI_LED();

  byte in_queue = Serial.available();

  // display only if queue isn't too long
  if (in_queue < MAX_DISP_QUEUE)
  {
    st.print(F("OFF "));
    char note_off_str[9] = "";
    snprintf(note_off_str, 9, "%02X %02X %02X", channel, pitch, velocity);
    st.println(note_off_str);

    st.show();
  }

  toggle_MIDI_LED();
}

void handlePitchBend(byte channel, int bend)
{
  toggle_MIDI_LED();

  byte in_queue = Serial.available();

  // display only if queue isn't too long
  if (in_queue < MAX_DISP_QUEUE)
  {
    st.print(F("BND "));
    char bend_str[8] = "";
    snprintf(bend_str, 8, "%02X %04X", channel, bend);
    st.println(bend_str);

    st.show();
  }

  toggle_MIDI_LED();
}

void handleControlChange(byte channel, byte number, byte value)
{
  toggle_MIDI_LED();

  byte in_queue = Serial.available();

  // display only if queue isn't too long
  if (in_queue < MAX_DISP_QUEUE)
  {
    st.print(F("CC  "));

    if (number == 123) {
      char cc_str[3] = "";
      snprintf(cc_str, 3, "%02X", channel);
      st.print(cc_str);
      st.println(F(" PANIC"));

    } else {
      char cc_str[9] = "";
      snprintf(cc_str, 9, "%02X %02X %02X", channel, number, value);
      st.println(cc_str);
    }
    st.show();
  }

  toggle_MIDI_LED();
}

void handleProgramChange(byte channel, byte number)
{
  toggle_MIDI_LED();

  // display only if queue isn't too long
  if (Serial.available() < MAX_DISP_QUEUE)
  {
    st.print(F("PC  "));
    char str[6] = "";
    snprintf(str, 6, "%02X %02X", channel, number);
    st.println(str);

    st.show();
  }

  toggle_MIDI_LED();
}


void handleSystemExclusive(byte* array, unsigned size);
void handleTimeCodeQuarterFrame(byte data);
void handleSongPosition(unsigned int beats);
void handleSongSelect(byte songnumber);

void handleTuneRequest(void)
{
  toggle_MIDI_LED();

  // display only if queue isn't too long
  if (Serial.available() < MAX_DISP_QUEUE)
  {
    st.println(F("Tun"));
    st.show();
  }

  toggle_MIDI_LED();
}

void handleClock(void)
{
  toggle_MIDI_LED();

  // display only if queue isn't too long
  if (Serial.available() < MAX_DISP_QUEUE)
  {
    st.println(F("Clk"));
    st.show();
  }

  toggle_MIDI_LED();
}

void handleStart(void)
{
  toggle_MIDI_LED();

  // display only if queue isn't too long
  if (Serial.available() < MAX_DISP_QUEUE)
  {
    st.println(F("START"));
    st.show();
  }

  toggle_MIDI_LED();
}

void handleContinue(void)
{
  toggle_MIDI_LED();

  // display only if queue isn't too long
  if (Serial.available() < MAX_DISP_QUEUE)
  {
    st.println(F("Cnt"));
    st.show();
  }

  toggle_MIDI_LED();
}

void handleStop(void)
{
  toggle_MIDI_LED();

  // display only if queue isn't too long
  if (Serial.available() < MAX_DISP_QUEUE)
  {
    st.println(F("STOP"));
    st.show();
  }

  toggle_MIDI_LED();
}

void handleActiveSensing(void)
{
  toggle_MIDI_LED();

  // display only if queue isn't too long
  if (Serial.available() < MAX_DISP_QUEUE)
  {
    st.println(F("Sens"));
    st.show();
  }

  toggle_MIDI_LED();
}

void handleSystemReset(void)
{
  toggle_MIDI_LED();

  // display only if queue isn't too long
  if (Serial.available() < MAX_DISP_QUEUE)
  {
    st.println(F("RESET"));
    st.show();
  }

  toggle_MIDI_LED();
}
