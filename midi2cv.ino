#include <MIDI.h>
#include "noteList.h"
//#include "pitches.h"

MIDI_CREATE_DEFAULT_INSTANCE();


//#define USE_UART 0




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

//static const unsigned gatePin = 2;

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

// 5 octaves -> 60 semitones (0..59)
#define MIN_SEMITONE 0
#define MAX_SEMITONE 59



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










byte unsigned GatePin[3];

/*
static const unsigned Gate1Pin     = 2;
static const unsigned Gate2Pin     = 3;
static const unsigned Gate3Pin     = 4;
static const unsigned Gate4Pin     = 5;
static const unsigned Gate5Pin     = 6;
static const unsigned Gate6Pin     = 7;
*/



















static const unsigned sMaxNumNotes = 16;

// 3 MidiNoteList for 3 MIDI channels 1, 2 and 3
// channel 1 is midiNotes[0]
// channel 2 is midiNotes[1]
// channel 3 is midiNotes[2]

MidiNoteList<sMaxNumNotes> midiNotes[3];



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
            // CV output here

            if (isFirstNote)
            {
                handleGateChanged(true, channel);
            }
            else
            {
                pulseGate(channel); // Retrigger envelopes. Remove for legato effect.
            }
        }
    }
}

// -----------------------------------------------------------------------------

void handleNoteOn(byte inChannel, byte inNote, byte inVelocity)
{

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
        default:
            break;
    }
}

void handleNoteOff(byte inChannel, byte inNote, byte inVelocity)
{
    midiNotes[inChannel - 1].remove(inNote);
    handleNotesChanged(false, inChannel);
}

// -----------------------------------------------------------------------------

void setup()
{
  #ifdef USE_UART
  // conflit entre MIDI et port série (mêmes pins, vitesse différente)
    //Serial.begin(115200);
    //uart_init();
    //stdout = &uart_output;
    //stdin  = &uart_input;
    //puts("UART init OK");
  #endif

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
/*
    pinMode(Gate1Pin, OUTPUT);
    pinMode(Gate2Pin, OUTPUT);
    pinMode(Gate3Pin, OUTPUT);
    pinMode(Gate4Pin, OUTPUT);
    pinMode(Gate5Pin, OUTPUT);
    pinMode(Gate6Pin, OUTPUT);
*/



  
//    pinMode(sGatePin,     OUTPUT);
//    pinMode(sAudioOutPin, OUTPUT);

    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);

    MIDI.begin(MIDI_CHANNEL_OMNI);
}

void loop()
{
    MIDI.read();
}
