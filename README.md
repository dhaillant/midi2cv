# midi2cv

midi2cv is a MIDI to Modular Analog Synth gateway.
It receives incoming MIDI from any DAW or MIDI compatible intrument and outputs Gates, Triggers and Analog Voltages (CV), depending on channels and Mode selected.

Several Modes are (will be) available:
* Mode 1: MIDI channels 1, 2 and 3 on Gates 2, 4, 6 and CV 1, 2 and 3, Mono-Last
* TBD
* Mode 6: MIDI channel 10, as drum channel, on all Gates and CVs.


Send a note on channel 1 with velocity 127:

amidi -p hw:2 -S '90 30 7F'

Should output 1V on DAC 1 and Gate 1 ON
