## Project

Live effects controller that you can control over or spike(!) across any intuitive direction for unique parameter control (used for audio in this case)!

https://github.com/legokito/spikebox/blob/main/spikebox-demo.mp4

## Goal 

I wanted something that was learnable and rigorously controllable over each parameter while also being fun and easy to intuitively mess with to where a kid could have fun with it - landed on this design. 

## Design Process 
- Love for automation while making music, and sudden effects in music styles like breakcore or glitch - "spiking" as a metaphor for physical movement. 
- Hate for absolute positioning due to variation errors it brings about, espcially for an IMU, so only used second or higher derivatives.
- Looked into balls, toys, foot-boards, and other input measures but the cube felt the most fun (you can throw it around!).
- (...I still have to work on fleshing this out)

# CAD
.stl files

# Electronics
esp32 c3 mini
adafruit bno085 (IMU)
6x custom capacitive touch pads (ref: https://learn.bela.io/tutorials/pure-data/sensors/diy-pressure-sensor/)

# Code
.ino file + imports





