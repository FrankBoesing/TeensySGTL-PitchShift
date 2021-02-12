// Simple WAV file player example with pitch shifting
// This example code is in the public domain.
// (c) Jan 2021 Frank B
// Teensy 4 variant

// Needs a potentiometer on the Audio Shield.

#if !defined(__IMXRT1062__)
#error Wrong MCU.
#endif


#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

AudioPlaySdWav           playWav1;
AudioOutputI2Sslave      audioOutput;
AudioConnection          patchCord1(playWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playWav1, 1, audioOutput, 1);
AudioControlSGTL5000     sgtl5000_1;

// Use these with the Teensy Audio Shield - SD Slot in AudioShield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

//#define SDCARD_CS_PIN    BUILTIN_SDCARD

const unsigned extMCLK = 11294118;
const unsigned maxSpeed = 65000;

void initSGTLasMaster(void);
void SGTLsetSampleFreq(const float freq);




void setup() {
  AudioMemory(4);
  initSGTLasMaster();
  Serial.begin(9600);
  sgtl5000_1.volume(0.7);

#if defined(SDCARD_SCK_PIN)
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
#endif  

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
  delay(10);

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

void initSGTLasMaster()
{

//enable MCLK-Output:
 
#include "utility/imxrt_hw.h"

  int fs = AUDIO_SAMPLE_RATE_EXACT;
  // PLL between 27*24 = 648MHz und 54*24=1296MHz
  int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
  int n2 = 1 + (24000000 * 27) / (fs * 256 * n1);

  double C = ((double)fs * 256 * n1 * n2) / 24000000;
  int c0 = C;
  int c2 = 10000;
  int c1 = C * c2 - (c0 * c2);
  set_audioClock(c0, c1, c2);
  // clear SAI1_CLK register locations
  CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI1_CLK_SEL_MASK))
       | CCM_CSCMR1_SAI1_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4
  CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
       | CCM_CS1CDR_SAI1_CLK_PRED(n1-1) // &0x07
       | CCM_CS1CDR_SAI1_CLK_PODF(n2-1); // &0x3f

  // Select MCLK
  IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1
    & ~(IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK))
    | (IOMUXC_GPR_GPR1_SAI1_MCLK_DIR | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL(0));  
  // enable Pin
  CORE_PIN23_CONFIG = 3;
  sgtl5000_1.enable(extMCLK, AUDIO_SAMPLE_RATE_EXACT * 4096.0L); //Use SGTL I2S Master Mode

}
