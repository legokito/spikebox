# Spikebox!
Live effects controller that you can control over or spike(!) across any intuitive direction for unique parameter control (used for audio in this case)!

https://github.com/user-attachments/assets/6cc97771-afb4-4d0b-955f-eef657490ff3

## Goal / How it Works 
I wanted an effects controller that was both precise enough for intentional performance and intuitive enough that anyone could pick it up and have fun with - this design ended up striking that balance perfectly! 

The pressure pads provide intentional, continuous control over effects such as reverb, bitcrush, delay, low-cut/high-cut, etc., while motion gestures (throws, jerks and orientation changes) trigger more dramatic changes such as timbral changes, or "laser-glitch" type effects by chaining together multiple audio-effects in ableton to modify the sound in unique ways. 

The purple pad acts as a hold control. Pressing it while other pads are being pressed latches their current values, allowing the effects to continue stably without maintaining pressure. If you press an individual pad to a pressure level past the one it was held at, it removes the latch for that effect and lets you continue performing it like normal. Pressing the purple pad again, while no other pads actively pressed releases any held values and returns the controller to its original mode. 

## Design Process 
- Love for automation while making music, and sudden effects in music styles like breakcore or glitch - "spiking" as a metaphor for physical movement. 
- Looked into balls, toys, foot-boards, and other input measures but the cube felt the most fun (you can throw it around!).
- (...I still have to work on fleshing this out)

### CAD
Designed a box that was:  
- Small
- Easily openable (but rigid while performing) 
- Could perfectly store all the electronics  
(check .stl files)

### Electronics
- ESP32 c3 mini - microcontroller of choice (small form factor, cheap, reliable)  
- Adafruit bno085 (IMU) - motion sensing, made heavy use of the accelerometer and gravity data (honestly this was overkill for my use case but we had it in the lab and it seemed fun to work with! It does open up interesting possibilities for extensions - I've already been eyeing the gyroscope as an extra axis of control.)
- 6x custom capacitive touch pads - used for pressure sensing (ref: https://learn.bela.io/tutorials/pure-data/sensors/diy-pressure-sensor/)  
- 300 mAh LiPo battery (because who wants a wired instrument....)

### Code
- Processes pressure sensor + IMU data in real time and sends MidiCC signals out.
- State machine with hysteresis to capture peak force of each pad while preventing noisy retriggering. 
- Lightweight signal processing (peak detection, exponential smoothing, log-scaling, and normalization - all for different parts) to convert noisy data into smoother and more expressive performance control.
- Modified BLEMIDI_Transport.h to batch MIDI CC messages into BLE packets before flushing, reducing Bluetooth overhead, latency jitter, and audio dropouts (had those happen and it was a pain to troubleshoot). 
(check .ino file)

### MaxMSP 
- Patch that lets you isolate each MidiCC signal from the ESP32 so you could manually map it in Ableton (I wish ableton had a better way to code this in rather than manually mapping it. Potential MaxForLive device idea...)
<img width="1211" height="806" alt="max patch mapper" src="https://github.com/user-attachments/assets/c360c17f-6adc-4e93-be4e-2855b4dc99df" />

### Ableton 
- 3 instruments in an instrument rack that get blended in volume based on the log db-scale
- Massive effect rack where different parameters map to macros (not pictured) where each macro is controlled incoming MidiCC signals. Pad presses generally controlled one to three parameters, while motion/“spike” gestures controlled multiple parameters simultaneously to create the heavily processed glitch effects.
<img width="876" height="185" alt="synths" src="https://github.com/user-attachments/assets/7048b135-894c-427e-a7e7-4dde8d049833" />
<img width="1301" height="109" alt="effect rack" src="https://github.com/user-attachments/assets/e6815d71-c829-4ba4-90c0-82958e66adca" />





