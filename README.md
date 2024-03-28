# midi2cv

midi2cv is a MIDI to Analog Synth gateway.
It receives incoming MIDI messages from any DAW or MIDI compatible instrument and outputs Gates, Triggers and Analog Voltages (CV), depending on selected modes and channels.

Several Modes are (will be) available (TBD)

How to test:
Send a note on channel 1 with velocity 127:

amidi -p hw:2 -S '90 30 7F'

Should output 1V on DAC 1 and Gate 1 ON

## MIDI monitor
MIDI monitor is a side project, it uses an I²C OLED screen to display as many incoming MIDI messages it can.
It's a usefull tool to help debug MIDI communication.
