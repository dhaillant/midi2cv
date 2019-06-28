# midi2cv



Send a note on channel 1 with velocity 127
amidi -p hw:2 -S '90 30 7F'
Should output 1V on DAC 1 and Gate 1 ON
