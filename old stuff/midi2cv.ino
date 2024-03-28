#include <MIDI.h>
#include "noteList.h"


MIDI_CREATE_DEFAULT_INSTANCE();



/*
 * Planned features:
 * -----------------
 * Chan 1: Dac 1 / Gates 1&2
 * Chan 2: Dac 2 / Gates 3&4
 * Chan 3: Dac 3 / Gates 5&6
 * Chan 4: Poly
 * Chan 5: Dac 1 tune, Dac 2 velocity, Dac 3 pitch bend
 * Chan 6: Dac 1 tune, Dac 2 velocity, Dac 3 control
 * Chan 10: 9 Drums on Gates 1~6 and Dac 1~3 (with velocity)
 */

byte mode = 0 ;
// mode from 0 to 5 = 6 modes
#define MAX_MODE 6


// SPI driver
#include "HRL_SPI.h"

// 12 bits SPI DAC MCP4921
#include "HRL_MCP492x.h"
#define MAX_DAC_DATA 0x0FFF
#define MIN_DAC_DATA 0

/*
 * 
 * bit 15 A/B: DACA or DACB Select bit
 *   1 = Write to DACB
 *   0 = Write to DACA
 * bit 14 BUF: VREF Input Buffer Control bit
 *   1 = Buffered
 *   0 = Unbuffered
 * bit 13 GA: Output Gain Select bit
 *   1 = 1x (VOUT = VREF * D/4096)
 *   0 = 2x (VOUT = 2 * VREF * D/4096)
 * bit 12 SHDN: Output Power Down Control bit
 *   1 = Output Power Down Control bit
 *   0 = Output buffer disabled, Output is high impedance
 * bit 11-0 D11:D0: DAC Data bits
 *   12 bit number “D” which sets the output value. Contains a value between 0 and 4095.
 * 
*/

// *********** 0.2 specific **************************************************************

// in Hardware V0.2, the Voltage reference is 2.5V x 2

int dac_config = 0x50;
// 0101 0000
// DACA BUF 2x SHDN

int dac_data = 0;

/*
 *   12 semitones per octave
 *  1 volt per octave
 *  1 semitone = 1 / 12 = 0.0833333333 V
 * 
 *  DAC full scale : 4096
 *  Voltage reference = 2 x 2.5 V
 *  DAC resolution = (2 x 2.5) / 4096 = 0.001220703125 V
 *  semitone resolution = 0.0833333 / 0.001220703125 = 68.2666667 steps
 * 
 *  5 V -> 5 octaves -> 60 half tones
 *  real full scale : 60 x 68 = 4080
 *  error on full scale : 16 x 0.001220703125 = 0.01953125 V
*/

// DAC value for a semitone
#define SEMI_TONE 68

// with a power supply of 0..5V, we can reach only 5 octaves on CV output
// 5 octaves -> 60 semitones (0..59)
#define MIN_SEMITONE 0
#define MAX_SEMITONE 59

// by default, the lowest note (0V) is C2 (MIDI #36)
byte base_note = 36;

void update_dac_output(int setpoint, uint8_t dac)
{
  //uint8_t dac_data;
  dac_data = setpoint * SEMI_TONE;
  writeMCP492x(dac_data, dac_config, dac);

  #ifdef USE_UART
  //printf("dac_data = %d\n", dac_data);
  #endif
}

// Front Panel Buttons. BTN1 on left
#define BTN1 A1
#define BTN2 A2

// *********** END of 0.2 specific ********************************************************



// array of pin numbers for Gate outputs
// this array values will depend on MODE
byte unsigned GatePin[6];

// array of pin numbers for Mode LEDs
byte unsigned ModeLEDPin[6];


static const unsigned sMaxNumNotes = 16;

// 3 MidiNoteList for 3 MIDI channels 1, 2 and 3
// channel 1 is midiNotes[0]
// channel 2 is midiNotes[1]
// channel 3 is midiNotes[2]

MidiNoteList<sMaxNumNotes> midiNotes[3];


void blink_MIDI_LED(void)
{
  // pin PC0 is connected to MIDI activity LED
  PORTC ^= (1 << 0);
}


// -----------------------------------------------------------------------------

inline void handleGateChanged(bool inGateActive, byte channel)
{
    // if inGateActive is true, outputs HIGH, else, outputs LOW
    digitalWrite(GatePin[channel - 1], inGateActive ? HIGH : LOW);
}

inline void pulseGate(byte channel)
{
    /*
     * Outputs LOW then 1ms later outputs HIGH on corresponding Channel's Gate pin
     */
    handleGateChanged(false, channel);
    delay(1);
    handleGateChanged(true, channel);
}


inline void handleDrumGateChanged(bool inGateActive, byte drum)
{
    // if inGateActive is true, outputs HIGH, else, outputs LOW
    digitalWrite(GatePin[drum - 1], inGateActive ? HIGH : LOW);
}


// -----------------------------------------------------------------------------

void handleNotesChanged(bool isFirstNote = false, byte channel = 1)
{
    if (midiNotes[channel - 1].empty())  // if midiNotes is empty
    {
        handleGateChanged(false, channel);
    }
    else
    {
      if (channel == 10)
      {
        
        
      }
      else
      {
        // Possible playing modes:
        // Mono Low:  use midiNotes.getLow
        // Mono High: use midiNotes.getHigh
        // Mono Last: use midiNotes.getLast

        byte currentNote = 0;
        if (midiNotes[channel - 1].getLast(currentNote))
        {
            if (isFirstNote)
            {
                handleGateChanged(true, channel);
            }
            else
            {
                pulseGate(channel); // Retrigger envelopes. Remove for legato effect.
            }

            if ((currentNote - base_note) > MAX_SEMITONE)
            {
              // example: if we receive note 100, we need to check if it's higher or not to the max note the DAC can produce
              // note - base_note=?
              // 100-36=64
              // 64 is higher than 59
              // 59+36=95
              // so we trim note at max_note + base_note
              currentNote = MAX_SEMITONE + base_note + 1;
            }
            
            if ((currentNote - base_note) < MIN_SEMITONE)
            {
              currentNote = MIN_SEMITONE + base_note;
            }

            // CV output here
            update_dac_output(currentNote - base_note, channel - 1);
         }
      }
    }
}

// -----------------------------------------------------------------------------

void handleNoteOn(byte inChannel, byte inNote, byte inVelocity)
{
    blink_MIDI_LED();
    
    switch(inChannel)
    {
        case 1:
        case 2:
        case 3:
            const bool firstNote = midiNotes[inChannel - 1].empty();
            // true if empty

            midiNotes[inChannel - 1].add(MidiNote(inNote, inVelocity));
            handleNotesChanged(firstNote, inChannel);

            break;
        case 10:
            // drum channel

            
        default:
            break;
    }
    
    delay(1);
    blink_MIDI_LED();
}

void handleNoteOff(byte inChannel, byte inNote, byte inVelocity)
{
    blink_MIDI_LED();

    midiNotes[inChannel - 1].remove(inNote);
    handleNotesChanged(false, inChannel);

    delay(1);
    blink_MIDI_LED();
}


void allNotesOff(byte inChannel)
{
  //digitalWrite(2, HIGH);
  byte currentNote = 0;

  // loop until note list is empty
  while(midiNotes[inChannel - 1].empty() == false)
  {
    // get the last note in the list
    midiNotes[inChannel - 1].getLast(currentNote);
    // remove that note from the list
    midiNotes[inChannel - 1].remove(currentNote);
  }

  // stop Gate output
  handleGateChanged(false, inChannel);
  // reset DAC ouput
  update_dac_output(0, inChannel - 1);
  //digitalWrite(2, LOW);
}

void handleControlChange(byte inChannel, byte inNumber, byte inValue)
{
  blink_MIDI_LED();

  switch(inNumber)
  {
    case 123:   // Channel Mode Message - All Notes Off
      allNotesOff(inChannel);
      break;
    default:
      break;
  }

  delay(1);
  blink_MIDI_LED();
}

// -----------------------------------------------------------------------------

void activate_mode(byte inMode)
{
  mode = inMode;
  switch (mode)
  {
    case 0:
      /*
       * Mode 0 is:
       * Chan 1: Dac 1 / Gates 1&2
       * Chan 2: Dac 2 / Gates 3&4
       * Chan 3: Dac 3 / Gates 5&6
       * 
       * Mono Last
       */

      // mapping Gates to pins
      GatePin[0] = 3;
      GatePin[1] = 5;
      GatePin[2] = 7;

      break;
    case 5:
      /*
       * Mode 5 is:
       * Chan 10: Gates 1 to 6 plus DAC 1 to 3 with velocity
       * 
       * Poly
       */

      // mapping Gates to pins
      for (byte i = 0; i < 6; i++)
      {
        GatePin[i] = i + 2;
      }

      break;
    default:
      break;
  }    
}

void setup()
{
    // Initiate SPI in Mode 0 with MSB first, NO interrupts and a clock of F_CPU/2 
    setupSPI(SPI_MODE_0, SPI_MSB, SPI_NO_INTERRUPT, SPI_MASTER_CLK2);

    // Setup DACs is selecting the SPI "Chip Select" pins for each DAC chip. DAC_PIN + offset (DAC_PIN is PB0, )
    SETUP_DAC(0);   // first DAC: PB0
    SETUP_DAC(1);   // second DAC: PB1
    SETUP_DAC(2);   // third DAC: PB2

    // initialise DAC outputs at 0
    update_dac_output(0, 0);
    update_dac_output(0, 1);
    update_dac_output(0, 2);

    // The midi2cv will react differently, depending on the selected mode.
    // By default, mode 0 is selected
    activate_mode(0);
    
    // 6 pins (PD2 to PD7) are digital outputs.
    for (byte i = 0; i < 6; i++)
    {
      // mapping Mode LEDs
      ModeLEDPin[i] = 2 + i;

      // set the pins as Outputs
      pinMode(ModeLEDPin[i], OUTPUT);  // active on HIGH
    }

    // Front Panel Buttons, active on LOW
    pinMode(BTN1, INPUT_PULLUP);
    pinMode(BTN2, INPUT_PULLUP);
    // /!\ Important: No hardware debounce. Software debounce required.

    // MIDI activity LED, active on LOW
    pinMode(A0, OUTPUT);  // LED is ON by default

    // quick "Hello" flashing
    blink_MIDI_LED();  // LED is OFF
    delay(100);
    blink_MIDI_LED();  // LED is ON
    delay(100);
    blink_MIDI_LED();  // LED is OFF
    delay(100);
    blink_MIDI_LED();  // LED is ON
    delay(100);
    blink_MIDI_LED();  // LED is OFF


    // Attach Callback functions to handle various MIDI incoming messages
    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);
    MIDI.setHandleControlChange(handleControlChange);

    // Start listening ALL MIDI channels
    MIDI.begin(MIDI_CHANNEL_OMNI);
}

#define BTN1_PRESSED (digitalRead(BTN1) == LOW)
#define BTN2_PRESSED (digitalRead(BTN2) == LOW)

void reset_LEDs(void)
{
  for (byte i = 0; i < MAX_MODE; i++)
  {
    digitalWrite(ModeLEDPin[i], LOW);
  }
}

void show_current_mode(void)
{
  reset_LEDs();
  digitalWrite(ModeLEDPin[mode], HIGH);
}

void debounce(void)
{
  delay(20);
}

void cycle_mode(void)
{
  mode++;
  if (mode >= MAX_MODE)
  {
    mode = 0;
  }
  show_current_mode();
}

void panic(void)
{
  // removes all stored notes

  // for each channel
  for (byte i = 1; i < 4; i++)
  {
    allNotesOff(i);
  }
}

//byte btn2_state = RELEASED;

void loop()
{
  // Managing Button presses
  if (BTN1_PRESSED)
  {
    panic();
    /*
     * while button pressed
     *  show current mode
     *  if button 2 pressed (detect rise)
     *    debounce
     *    cycle mode
     *   
     */
    show_current_mode();
    while BTN1_PRESSED
    {
      if (BTN2_PRESSED)
      {
        cycle_mode();
        debounce();
        while (BTN2_PRESSED);
        debounce();
      }
    }
    reset_LEDs();
  }

  // PANIC?
  // if both buttons are pressed,
  //if ((digitalRead(BTN1) == LOW) && (digitalRead(BTN2) == LOW))

  // if either buttons are pressed,
  // if (BTN1_PRESSED || BTN2_PRESSED)
  if (BTN2_PRESSED)
  {
    panic();
  }



  MIDI.read();
}
