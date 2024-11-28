# midi2cv

midi2cv is a MIDI to Analog Synth gateway.
It receives incoming MIDI messages from any DAW or MIDI compatible instrument and outputs Gates, Triggers and Analog Voltages (CV), depending on selected modes and channels.

Several Modes are (will be) available (TBD)

How to test:
Send a note on channel 1 with velocity 127:

amidi -p hw:2 -S '90 30 7F'

Should output 1V on DAC 1 and Gate 1 ON

## MIDI monitor
MIDI monitor is a side project, it uses an SPI (I²C in HW < 0.6) OLED screen to display as many incoming MIDI messages it can.
It's a useful tool to help debug MIDI communication.
Latest addition includes SysEx handling.

## Euclid
Euclid is the adaptation of HAGIWO's Euclidian Rhythm Sequencer (ユークリッドリズムシーケンサー), but for MIDI.
This version uses MIDI Clock messages (24 PPQN) instead of Trig Pulses.
Original code and documentation: https://note.com/solder_state/n/n433b32ea6dbc
