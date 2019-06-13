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

// *********** END of 0.2 specific ********************************************************



// array of pin numbers for Gate outputs
byte unsigned GatePin[3];


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

// -----------------------------------------------------------------------------

void handleNotesChanged(bool isFirstNote = false, byte channel = 1)
{
    if (midiNotes[channel - 1].empty())  // if midiNotes is empty
    {
        handleGateChanged(false, channel);
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

// -----------------------------------------------------------------------------

void setup()
{

    // Initiate SPI in Mode 0 with MSB first, NO interrupts and a clock of F_CPU/2 
    setupSPI(SPI_MODE_0, SPI_MSB, SPI_NO_INTERRUPT, SPI_MASTER_CLK2);

    SETUP_DAC(0);   // first DAC
    SETUP_DAC(1);   // second DAC
    SETUP_DAC(2);   // third DAC

    GatePin[0] = 3;
    GatePin[1] = 5;
    GatePin[2] = 7;
    pinMode(GatePin[0], OUTPUT);
    pinMode(GatePin[1], OUTPUT);
    pinMode(GatePin[2], OUTPUT);

    // MIDI activity LED
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



    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);

    MIDI.begin(MIDI_CHANNEL_OMNI);
}

void loop()
{
    MIDI.read();
}
