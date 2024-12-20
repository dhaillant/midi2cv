/*
 * ユークリッドリズムシーケンサー
 * HAGIWO's Euclidian Rhythm Sequencer, for MIDI Clock
 * 
 * This version uses MIDI Clock messages instead of Trig Pulses
 * 
 * Original code and documentation: https://note.com/solder_state/n/n433b32ea6dbc
 * 
 * License of original euclid_pu.ino by HAGIWO is:
 * "My source code and schematic rights are completely free.
 * Therefore, I do not include any rights notes in the documentation."
 * https://lookmumnocomputer.discourse.group/t/fm-vco-with-arduino-hagiwo/3566/10
 * 
 * Additions and modifications by David Haillant are under GPL V2.0
 * 
 * Additional (included) libraries (if present) are under their own licenses. Check individual library for details.
 * 
 * 
 * This version is compatible with David Haillant's MIDI2CV (HW Ver. 0.5.1)
 * https://github.com/dhaillant/midi2cv
 * 
 */

//#define MIDI8D_HW
//#define MDI2CV_HW_05
#define MDI2CV_HW_06

#ifdef MIDI8D_HW
  #define MIDI8D
  #define SIX_DIGITAL_OUTPUTS
#endif

#ifdef MDI2CV_HW_05
  #define MIDI2CV
  #define I2C_DISPLAY
  #define DACS
#endif

#ifdef MDI2CV_HW_06
  #define MIDI2CV
  #define SPI_DISPLAY
  #define DACS
#endif

// If using a SPI display (HW >= 0.6) comment the following line
//#define I2C_DISPLAY
// I²C needs 29 more bytes than SPI

// If 6 digital outputs are available, uncomment the following line:
//#define SIX_DIGITAL_OUTPUTS

// If 2 SPI DACs are available, uncomment the following line: (16 bytes)
//#define DACS


// MIDI *************************************
#define MIDI_BAUD_RATE 31250
#define MIDI_IN 0
#define MIDI_LED A3

#define MIDI_BYTE_CLOCK 0xF8
#define MIDI_BYTE_START 0xFA
#define MIDI_BYTE_STOP 0xFC
#define MIDI_BYTE_CONTINUE 0xFB

uint8_t midi_clock_counter = 0;

void toggle_MIDI_LED(void)
{
  // pin PC3 is connected to MIDI activity LED
  PORTC ^= (1 << 3);
}

// screen *************************************
/*
  OLED Displays require 1 byte for every 8 pixels
  The 128x64 version requires 1K of SRAM
*/
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#ifdef I2C_DISPLAY
  #include <Wire.h>

  #define OLED_ADDRESS 0x3C

  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#else
  #include <SPI.h>

  #ifdef SOFT_SPI
    // Declaration for SSD1306 display connected using software SPI (default case):
    #define OLED_COPI  11
    #define OLED_CLK   13
    #define OLED_DC    7
    #define OLED_CS    8
    #define OLED_RESET 6
    Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
      OLED_COPI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
  #else  
    // Declaration for SSD1306 display connected using hardware SPI
    #define OLED_DC     7
    #define OLED_CS     8
    #define OLED_RESET  6
    Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
      &SPI, OLED_DC, OLED_RESET, OLED_CS);
  #endif
#endif



// Keys *************************************
#define UP_PIN A0
#define ENTER_PIN A1
#define DOWN_PIN A2

#define MAX_EUCLID_OUTPUTS 6        // number of Euclidian outputs

// GATEs *************************************
#define GATE1_PIN 2          // D2
#define GATE2_PIN 3          // D3
#define GATE3_PIN 4          // D4
#define GATE4_PIN 5          // D5

#ifdef SIX_DIGITAL_OUTPUTS
  #define GATE5_PIN 6          // D6
  #define GATE6_PIN 7          // D7
#endif


//#define DACS
#ifdef DACS
  // DACs ************************************
  #include <MCP48xx.h>
  
  // Define the MCP4822 instance, giving it the CS (Chip Select) pin
  // The constructor will also initialize the SPI library
  // We can also define a MCP4812 or MCP4802
  MCP4822 dac1(9);
  MCP4822 dac2(10);
  
  // in mV
  #define DAC_MAX_VAL 2500
  #define DAC_MIN_VAL 0
#endif



//#define CLOCK_IN_PIN 0

#define KEYDETECTOR
#ifdef KEYDETECTOR
  #include <KeyDetector.h>

  #define KEY_UP 1
  #define KEY_ENTER 2
  #define KEY_DOWN 3

  Key keys[] = {{KEY_UP, UP_PIN}, {KEY_ENTER, ENTER_PIN}, {KEY_DOWN, DOWN_PIN}};

  KeyDetector myKeyDetector(keys, sizeof(keys)/sizeof(Key), 10, 0, true);
  // true is for PULLUP
#endif

//each channel param
byte hits[6] = { 4, 4, 5, 3, 2, 16};//each channel hits
byte offset[6] = { 0, 2, 0, 8, 3, 9};//each channele step offset
bool mute[6] = {0, 0, 0, 0, 0, 0}; //mute 0 = off , 1 = on
byte limit[6] = {16, 16, 16, 16, 16, 16};//eache channel max step

//Sequence variable
byte j = 0;
byte k = 0;
byte m = 0;
byte buf_count = 0;

const static byte euc16[17][16] PROGMEM = {//euclidian rythm
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
  {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
  {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0},
  {1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0},
  {1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0},
  {1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0},
  {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0},
  {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0},
  {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1},
  {1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1},
  {1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1},
  {1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1},
  {1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};
bool offset_buf[6][16];//offset buffer , Stores the offset result

bool trg_in = 0;//external trigger in H=1,L=0
bool old_trg_in = 0;
byte playing_step[6] = {0, 0, 0, 0, 0, 0}; //playing step number , CH1,2,3,4,5,6
unsigned long gate_timer = 0;//countermeasure of sw chattering

//display param
byte select_menu = 0;//0=CH,1=HIT,2=OFFSET,3=LIMIT,4=MUTE,5=RESET,
byte select_ch = 0;//0~5 = each channel -1 , 6 = random mode
bool disp_reflesh = 1;//0=not reflesh display , 1= reflesh display , countermeasure of display reflesh bussy

const byte graph_x[6] = {0, 40, 80, 15, 55, 95};//each chanel display offset
const byte graph_y[6] = {0, 0,  0,  32, 32, 32};//each chanel display offset

byte line_xbuf[17];//Buffer for drawing lines
byte line_ybuf[17];//Buffer for drawing lines

const byte x16[16] = {15,  21, 26, 29, 30, 29, 26, 21, 15, 9,  4,  1,  0,  1,  4,  9};//Vertex coordinates
const byte y16[16] = {0,  1,  4,  9,  15, 21, 26, 29, 30, 29, 26, 21, 15, 9,  4,  1};//Vertex coordinates


//random assign
const byte hit_occ[6] = {0, 10, 20, 40, 40, 20}; //random change rate of occurrence
const byte off_occ[6] = {0, 20, 20, 20, 40, 20}; //random change rate of occurrence
const byte mute_occ[6] = {20, 20, 20, 20, 20, 20}; //random change rate of occurrence
const byte hit_rng_max[6] = {0, 6, 16, 8, 9, 16}; //random change range of max
const byte hit_rng_min[6] = {0, 1, 6, 1, 5, 10}; //random change range of max

byte bar_now = 1;//count 16 steps, the bar will increase by 1.
const byte bar_max[4] = {2, 4, 8, 16} ;//selectable bar
byte bar_select = 1;//selected bar
byte step_cnt = 0;//count 16 steps, the bar will increase by 1.


void write_output(uint8_t n, uint8_t value)
{
  #ifdef MIDI2CV
    switch (n)
    {
      case 0:
        dac1.setVoltageA((value == HIGH) ? DAC_MAX_VAL : DAC_MIN_VAL);
        dac1.updateDAC();
        //digitalWrite(GATE1_PIN, value);
        break;
      case 1:
        digitalWrite(GATE1_PIN, value);
        break;
      case 2:
        dac1.setVoltageB((value == HIGH) ? DAC_MAX_VAL : DAC_MIN_VAL);
        dac1.updateDAC();
        //digitalWrite(GATE3_PIN, value);
        break;
      case 3:
        digitalWrite(GATE2_PIN, value);
        break;
      case 4:
        dac2.setVoltageA((value == HIGH) ? DAC_MAX_VAL : DAC_MIN_VAL);
        dac2.updateDAC();
        //digitalWrite(GATE5_PIN, value);
        break;
      case 5:
        digitalWrite(GATE3_PIN, value);
        break;
    }
  #endif
  #ifdef MIDI8D
    switch (n)
    {
      case 0:
        digitalWrite(GATE1_PIN, value);
        break;
      case 1:
        digitalWrite(GATE2_PIN, value);
        break;
      case 2:
        digitalWrite(GATE3_PIN, value);
        break;
      case 3:
        digitalWrite(GATE4_PIN, value);
        break;
      case 4:
        digitalWrite(GATE5_PIN, value);
        break;
      case 5:
        digitalWrite(GATE6_PIN, value);
        break;
    }
  #endif
}




void setup() {
  // OLED setting
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  OLED_display();

  //pin mode setting
  pinMode(UP_PIN,     INPUT_PULLUP);
  pinMode(ENTER_PIN,  INPUT_PULLUP);
  pinMode(DOWN_PIN,   INPUT_PULLUP);
  
  pinMode(GATE1_PIN,  OUTPUT); //CH1
  pinMode(GATE2_PIN,  OUTPUT); //CH2
  pinMode(GATE3_PIN,  OUTPUT); //CH3
  pinMode(GATE4_PIN,  OUTPUT); //CH4
  #ifdef SIX_DIGITAL_OUTPUTS
    pinMode(GATE5_PIN,  OUTPUT); //CH5
    pinMode(GATE6_PIN,  OUTPUT); //CH6
  #endif

  #ifdef DACS
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
  
    dac1.setVoltageA(0);
    dac1.setVoltageB(0);
    dac2.setVoltageA(0);
    dac2.setVoltageB(0);
  
    // We send the command to the MCP4822
    // This is needed every time we make any change
    dac1.updateDAC();
    dac2.updateDAC();
  #endif  


  // Set MIDI baud rate:
  Serial.begin(MIDI_BAUD_RATE);

  pinMode(MIDI_LED,   OUTPUT);
  digitalWrite(MIDI_LED, LOW);
  delay(1000);
  digitalWrite(MIDI_LED, HIGH);
}

void loop() {
  old_trg_in = trg_in;
  trg_in = 0;         // by default, Trig OFF

  // read serial port for incoming MIDI messages
  // if message present, retrieve one byte
  // if the byte is a RealTime command (one byte long) then
  // increment a counter
  // divide the counter value by 6 and rise a flag when modulo is 0
  // reset the counter at every bar
  uint8_t inByte = 0;
  if (Serial.available() > 0) {
    // get incoming byte:
    inByte = Serial.read();

    switch (inByte) {
      case MIDI_BYTE_CLOCK:
        midi_clock_counter = midi_clock_counter % 96; // this will reset the clock_step to 0 after 96 ppqn are received,
        //assuming your MIDI clock data is being sent at the standard 24ppqn that will reset clock_step every bar.

        toggle_MIDI_LED();
        if (midi_clock_counter % 6 == 0)  // every 6 clock steps = one sixteenth
        {
          trg_in = 1;         // Trig ON
        }
        midi_clock_counter++;
        break;
      case MIDI_BYTE_START:
        reset_steps();
        break;
      case MIDI_BYTE_STOP:
        reset_steps();
        break;
      default:
        break;
    }
  }


  myKeyDetector.detect();

  if (myKeyDetector.trigger) {
    disp_reflesh = 1;
  }

  switch (myKeyDetector.trigger) {
    case KEY_UP:
      if (select_menu != 0) {
        select_menu --;
      }
      break;
    case KEY_DOWN:
      select_menu ++;
      break;
  }

  if (select_ch != 6) { // not random mode
    select_menu = constrain(select_menu, 0, 5);
  }
  else  if (select_ch == 6) { // random mode
    select_menu = constrain(select_menu, 0, 1);
  }

  //-----------------push button----------------------
  if (myKeyDetector.trigger == KEY_ENTER) { //push button on
    switch (select_menu) {
      case 0: //select chanel
        select_ch ++;
        if (select_ch >= 7) {
          select_ch = 0;
        }
        break;
      case 1: //hits
        if (select_ch != 6) { // not random mode
          hits[select_ch]++ ;
          if (hits[select_ch] >= 17) {
            hits[select_ch] = 0;
          }
        }
        else  if (select_ch == 6) { // random mode
          bar_select ++;
          if (bar_select >= 4) {
            bar_select = 0;
          }
        }
        break;
      case 2: //offset
        offset[select_ch]++ ;
        if (offset[select_ch] >= 16) {
          offset[select_ch] = 0;
        }
        break;
      case 3: //limit
        limit[select_ch]++ ;
        if (limit[select_ch] >= 17) {
          limit[select_ch] = 0;
        }
        break;
      case 4: //mute
        mute[select_ch] = !mute[select_ch] ;
        break;

      case 5: //reset
        reset_steps();
        break;
    }
  }

  //-----------------offset setting----------------------
  for (k = 0; k <= 5; k++) { //k = 1~6ch
    for (uint8_t i = offset[k]; i <= 15; i++) {
      offset_buf[k][i - offset[k]] = (pgm_read_byte(&(euc16[hits[k]][i]))) ;
    }

    for (uint8_t i = 0; i < offset[k]; i++) {
      offset_buf[k][16 - offset[k] + i] = (pgm_read_byte(&(euc16[hits[k]][i])));
    }
  }

  //-----------------trigger detect & output----------------------
  if (old_trg_in == 0 && trg_in == 1) {
    gate_timer = millis();
    for (uint8_t i = 0; i <= 5; i++) {
      playing_step[i]++;      //When the trigger in, increment the step by 1.
      if (playing_step[i] >= limit[i]) {
        playing_step[i] = 0;  //When the step limit is reached, the step is set back to 0.
      }
    }
    for (k = 0; k <= 5; k++) {//output gate signal
      if (offset_buf[k][playing_step[k]] == 1 && mute[k] == 0) {
        write_output(k, HIGH);
      }
    }
    disp_reflesh = 1;//Updates the display where the trigger was entered.If it update it all the time, the response of gate on will be worse.

    if (select_ch == 6) {// random mode setting
      step_cnt ++;
      if (step_cnt >= 16) {
        bar_now ++;
        step_cnt = 0;
        if (bar_now > bar_max[bar_select]) {
          bar_now = 1;
          Random_change();
        }
      }
    }
  }

  if  (gate_timer + 10 <= millis()) { //off all gate , gate time is 10msec
    for (uint8_t i = 0; i < MAX_EUCLID_OUTPUTS; i++)
    {
      write_output(i, LOW);
    }
  }


  if (disp_reflesh == 1) {
    OLED_display();//reflesh display
    disp_reflesh = 0;
  }
}

void Random_change() { // when random mode and full of bar_now ,
  for (k = 1; k <= 5; k++) {

    if (hit_occ[k] >= random(1, 100)) { //hit random change
      hits[k] = random(hit_rng_min[k], hit_rng_max[k]);
    }

    if (off_occ[k] >= random(1, 100)) { //hit random change
      offset[k] = random(0, 16);
    }

    if (mute_occ[k] >= random(1, 100)) { //hit random change
      mute[k] = 1;
    }
    else if (mute_occ[k] < random(1, 100)) { //hit random change
      mute[k] = 0;
    }
  }
}

void OLED_display() {
  display.clearDisplay();
  //-------------------------euclidean circle display------------------
  //draw setting menu
  display.setCursor(120, 0);
  if (select_ch != 6) { // not random mode
    display.print(select_ch + 1);
  }
  else if (select_ch == 6) { //random mode
    display.print(F("R"));
  }
  display.setCursor(120, 9);
  if (select_ch != 6) { // not random mode
    display.print(F("H"));
  }
  else if (select_ch == 6) { //random mode
    display.print("O");
  }
  display.setCursor(120, 18);
  if (select_ch != 6) { // not random mode
    display.print(F("O"));
    display.setCursor(0, 36);
    display.print(F("L"));
    display.setCursor(0, 45);
    display.print(F("M"));
    display.setCursor(0, 54);
    display.print(F("R"));
  }


  //random count square
  if (select_ch == 6) { //random mode
    //        display.drawRect(1, 32, 6, 32, WHITE);
    //    display.fillRect(1, 32, 6, 16, WHITE);
    display.drawRect(1, 62 - bar_max[bar_select] * 2, 6, bar_max[bar_select] * 2 + 2, WHITE);
    display.fillRect(1, 64 - bar_now * 2 , 6, bar_max[bar_select] * 2, WHITE);
  }
  //draw select triangle
  if ( select_menu == 0) {
    display.drawTriangle(113, 0, 113, 6, 118, 3, WHITE);
  }
  else  if ( select_menu == 1) {
    display.drawTriangle(113, 9, 113, 15, 118, 12, WHITE);
  }

  if (select_ch != 6) { // not random mode
    if ( select_menu == 2) {
      display.drawTriangle(113, 18, 113, 24, 118, 21, WHITE);
    }
    else  if ( select_menu == 3) {
      display.drawTriangle(12, 36, 12, 42, 7, 39, WHITE);
    }
    else  if ( select_menu == 4) {
      display.drawTriangle(12, 45, 12, 51, 7, 48, WHITE);
    }
    else  if ( select_menu == 5) {
      display.drawTriangle(12, 54, 12, 60, 7, 57, WHITE);
    }
  }

  //draw step dot
  for (k = 0; k <= 5; k++) { //k = 1~6ch
    for (j = 0; j <= limit[k] - 1; j++) { // j = steps
      display.drawPixel(x16[j] + graph_x[k], y16[j] + graph_y[k], WHITE);
    }
  }

  //draw hits line : 2~16hits
  for (k = 0; k <= 5; k++) { //ch count
    buf_count = 0;
    for (m = 0; m < 16; m++) {
      if (offset_buf[k][m] == 1) {
        line_xbuf[buf_count] = x16[m] + graph_x[k];//store active step
        line_ybuf[buf_count] = y16[m] + graph_y[k];
        buf_count++;
      }
    }

    for (j = 0; j < buf_count - 1; j++) {
      display.drawLine(line_xbuf[j], line_ybuf[j], line_xbuf[j + 1], line_ybuf[j + 1], WHITE);
    }
    display.drawLine(line_xbuf[0], line_ybuf[0], line_xbuf[j], line_ybuf[j], WHITE);
  }
  for (j = 0; j < 16; j++) {//line_buf reset
    line_xbuf[j] = 0;
    line_ybuf[j] = 0;
  }

  //draw hits line : 1hits
  for (k = 0; k <= 5; k++) { //ch count
    buf_count = 0;
    if (hits[k] == 1) {
      display.drawLine(15 + graph_x[k], 15 + graph_y[k], x16[offset[k]] + graph_x[k], y16[offset[k]] + graph_y[k], WHITE);
    }
  }

  //draw play step circle
  for (k = 0; k <= 5; k++) { //ch count
    if (mute[k] == 0) { //mute on = no display circle
      if (offset_buf[k][playing_step[k]] == 0) {
        display.drawCircle(x16[playing_step[k]] + graph_x[k], y16[playing_step[k]] + graph_y[k], 2, WHITE);
      }
      if (offset_buf[k][playing_step[k]] == 1) {
        display.fillCircle(x16[playing_step[k]] + graph_x[k], y16[playing_step[k]] + graph_y[k], 3, WHITE);
      }
    }
  }
  display.display();
}

void reset_steps(void)
{
  for (k = 0; k <= 5; k++) {
    playing_step[k] = 0;
  }
  midi_clock_counter = 0;
}
