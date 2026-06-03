#include <Wire.h>
#include <Adafruit_BNO08x.h>
#include <Arduino.h>
#include <BLEMIDI_Transport.h> // edited to flush more efficiently
#include <hardware/BLEMIDI_ESP32_NimBLE.h> // updated with code from here - https://github.com/lathoub/Arduino-BLE-MIDI/issues/96?utm_source=chatgpt.com#issuecomment-2575746451

Adafruit_BNO08x bno(-1);
sh2_SensorValue_t sensorValue;

BLEMIDI_CREATE_INSTANCE("ESP32-C3 MIDI", MIDI);

// setup output vector
enum {
  FSR0, FSR1, FSR2, FSR3, FSR4,
  GRAV_X, GRAV_Y, GRAV_Z,
  GYRO_X, GYRO_Y, GYRO_Z,
  LINA_X, LINA_Y, LINA_Z,
  NUM_PARAMS
};

const int NUM_FSR = 5; 

float values[NUM_PARAMS];

int recorder;
bool isRecording;

int baseline = 600;
float thresholds[NUM_FSR] = {baseline, baseline, baseline, baseline, baseline};
bool hasFallen[NUM_FSR] = {false, false, false, false, false};  //hysteresis for pad threshold locking 
int limit = 3600;

float spikeEnv[3];
float driftEnv[3];

//helpers
uint8_t toCC(float value, float minVal, float maxVal);
bool anyPadPressed(int NUM_FSR);

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9); // sda + scl
  Wire.setClock(400000);
  delay(3000);

  // scanning
  Serial.println("Scanning I2C...");
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found device at 0x");
      Serial.println(addr, HEX);
    }
  }

  //bno
  int attempts = 0;
  if (!bno.begin_I2C(0x4A, &Wire)) {
    Serial.println("bno not found!");
    while (attempts < 5){
      Serial.print("attempt");
      Serial.println(attempts);
      delay(200);
      if (bno.begin_I2C(0x4a, &Wire)){
        break;
      }
      else{
        attempts++;
      }
    }

  }
  if (!bno.enableReport(SH2_GRAVITY, 5000)) { // orientation determines mix level over some effects
    Serial.println("Could not enable gravity vector");
    while (1);
  }
  if (!bno.enableReport(SH2_GYROSCOPE_CALIBRATED, 20000)) { // this for recognizing when thrown or jerked across rotational axis or two... trigger when val > 8?
    Serial.println("Could not enable gyroscope");
    while (1);
  }
  if (!bno.enableReport(SH2_LINEAR_ACCELERATION, 20000)) { // drift overtime and is also spikeable
    Serial.println("Could not enable linear acceleration");
    while (1);
  }

  // fsrs
  pinMode(5, INPUT_PULLDOWN);
  isRecording = false;

  analogSetAttenuation(ADC_11db);

  delay(1000);
  MIDI.begin();

  // solution for bluetooth lagging issue - only flush buffer once full
  BLEMIDI.setDeferFlush(true);
}

void loop() {
  static unsigned long lastMicros = 0;
  unsigned long now = micros();
  float dt = (now - lastMicros) * 1e-6f;   // seconds
  lastMicros = now;

  float spikeTau = 0.65f;
  float spikeDecay = expf(-dt / spikeTau);

  float driftTau = 0.1f;
  float driftDecay = expf(-dt / driftTau);

  // understand why we swtiched from if to while - still fuzzy on this. 
  while (bno.getSensorEvent(&sensorValue)){
    switch (sensorValue.sensorId) {
      case SH2_GRAVITY:
        values[GRAV_X] = sensorValue.un.gravity.x;
        values[GRAV_Y] = sensorValue.un.gravity.y;
        values[GRAV_Z] = sensorValue.un.gravity.z;
        break;
      case SH2_GYROSCOPE_CALIBRATED:
        values[GYRO_X] = sensorValue.un.gyroscope.x;
        values[GYRO_Y] = sensorValue.un.gyroscope.y;
        values[GYRO_Z] = sensorValue.un.gyroscope.z;
        break;
      case SH2_LINEAR_ACCELERATION:
        values[LINA_X] = sensorValue.un.linearAcceleration.x;
        values[LINA_Y] = sensorValue.un.linearAcceleration.y;
        values[LINA_Z] = sensorValue.un.linearAcceleration.z;
        break;
    }
  }

  for (int p = 0; p < NUM_FSR; p++) {
    values[FSR0 + p] = analogRead(p);
  }

  recorder = digitalRead(5);

  /*
  fsr state machine
  */
  if (!isRecording){ 
    for (int i = 0; i < NUM_FSR; i++){
      if (values[i] < baseline){
        hasFallen[i] = true;
      }
      if (values[i] > thresholds[i] && hasFallen[i]){
        thresholds[i] = baseline;
      }
    }
    if (recorder == 1){ isRecording = true; }
  }

  if (isRecording){
    if (!anyPadPressed(NUM_FSR))  {
      for (int i = 0; i < NUM_FSR; i++){
        thresholds[i] = baseline;
      }
    }
    else{
      for (int i = 0; i < NUM_FSR; i++){
        if (values[i] > thresholds[i]) {
          thresholds[i] = values[i];
          hasFallen[i] = false;
        }
      }
    }

    if (recorder == 0){ isRecording = false; }
  }

  /*
  gravity
  */
  float sumOfSquares = values[GRAV_X] * values[GRAV_X] + values[GRAV_Y] * values[GRAV_Y] + values[GRAV_Z] * values[GRAV_Z];

  /*
  lin acc 
  */
  for (int i = 0; i < 3; i++){
    spikeEnv[i] = max(fabsf(values[LINA_X + i]), spikeEnv[i] * spikeDecay);
  }

  /*
  lin acc drift
  */
  for (int i = 0; i < 3; i++){
    driftEnv[i] += (1 - driftDecay) * (values[LINA_X + i] - driftEnv[i]);
  }

  /*
  TODO: gyro for rotation
  */

  /*
  output values 
  */
  static unsigned long lastSendMs = 0;
  if (millis() - lastSendMs >= 20) {
    lastSendMs = millis();

    //fsr
    for (int i = 0; i < NUM_FSR; i++) {
      uint8_t cc = toCC(max(thresholds[i], values[i]), baseline, limit);
      MIDI.sendControlChange(i + 1, cc, 1);
    }

    //gravity - scaling to allow for better mix blend (cause db scales in ableton are logarithmic)
    const float dbFloor = -60.0f;   
    for (int i = 0; i < 3; i++){
      uint8_t cc;
      if (sumOfSquares > 0.0001f) {   // div by 0
        float w = (values[GRAV_X + i] * values[GRAV_X + i]) / sumOfSquares;  
        float dB = 10.0f * log10f(max(w, 1e-7f)); 
        cc = toCC(dB, dbFloor, 0);
      }
      else
        cc = toCC(dbFloor, dbFloor, 0);   
      MIDI.sendControlChange(i + 1, cc, 2);
    }

    //lin acc
    for (int i = 0; i < 3; i++){
      uint8_t cc = toCC(spikeEnv[i], 12, 40);
      MIDI.sendControlChange(i + 1, cc, 3);
    }

    //lin acc drift
    for (int i = 0; i < 3; i++){
      float n = constrain(driftEnv[i] / 15.0f, -1.0f, 1.0f);  // normalize ±6 -> [-1,1]
      float e = n;                                    // scaling
      float s = 0.5f + 0.5f * e;                              // default to center
      uint8_t cc = toCC(s, 0, 1);
      MIDI.sendControlChange(i + 1, cc, 4);
    }

    BLEMIDI.flush();
  } 
}

// convert values to CC range - helper
uint8_t toCC(float value, float minVal, float maxVal) {
  float normalized = (value - minVal) / (maxVal - minVal);
  normalized = constrain(normalized, 0.0f, 1.0f);
  return (uint8_t)(normalized * 127.0f);
}

bool anyPadPressed(int NUM_FSR){
  for (int i = 0; i < NUM_FSR; i++){
    if (values[FSR1 + i] < baseline){
      continue;
    } 
    return true;
  }
  return false;
}