// Simple WAV file player example with pitch shifting
// This example code is in the public domain.
// (c) Jan 2021 Frank B

// Teensy LC
// Needs a potentionmeter on the Audio Board.


#if !defined(KINETISL) //Teensy LC
#error Wrong MCU.
#endif

#include <Audio.h>
#include <Wire.h>
#include <SD.h>

AudioPlaySdWav           playWav1;
AudioOutputI2Sslave      audioOutput;
AudioConnection          patchCord1(playWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playWav1, 1, audioOutput, 1);
AudioControlSGTL5000     sgtl5000_1;

// Use these with the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

const unsigned extMCLK = 16000000;
const unsigned maxSpeed = 51000;

void SGTLsetSampleFreq(const float freq);

void setup() {
  AudioMemory(4);
  Serial.begin(9600);
  sgtl5000_1.enable(); //Uses SGTL I2S Master Mode automatically
  sgtl5000_1.volume(0.7);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);

  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }

}

void playFile(const char *filename)
{
  Serial.print("Playing file: ");
  Serial.println(filename);

  playWav1.play(filename);
  delay(25);

  while (playWav1.isPlaying()) {
    float speed = analogRead(15);
    float freq = maxSpeed * (speed / 1024.0f);
    if (freq < extMCLK / 1024) freq = extMCLK / 1024;
    SGTLsetSampleFreq(freq);
    Serial.println(freq);
    delay(200);
  }
}


void loop() {
  playFile("SDTEST2.WAV");  // filenames are always uppercase 8.3 format
  delay(500);
  playFile("SDTEST3.WAV");
  delay(500);
  playFile("SDTEST4.WAV");
  delay(500);
  playFile("SDTEST1.WAV");
  delay(1500);
}

//SGTL-5000 registers
#define CHIP_PLL_CTRL      0x0032

bool writeSgtl(unsigned int reg, unsigned int val)
{
  Wire.beginTransmission(0x0A);
  Wire.write(reg >> 8);
  Wire.write(reg);
  Wire.write(val >> 8);
  Wire.write(val);
  if (Wire.endTransmission() == 0) return true;
  return false;
}

void SGTLsetSampleFreq(const float freq) {
  float pllFreq = 4096.0f * freq;
  uint32_t int_divisor = ((uint32_t)(pllFreq / extMCLK)) & 0x1f;
  uint32_t frac_divisor = (uint32_t)(((pllFreq / extMCLK) - int_divisor) * 2048.0) & 0x7ff;
  writeSgtl(CHIP_PLL_CTRL, (int_divisor << 11) | frac_divisor);
}
