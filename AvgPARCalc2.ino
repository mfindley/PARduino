/*
 AvgPARCalc
 
 Read the signal generated from a LI-COR Quantum PAR sensor and calculate an average signal.  Sensor measures photosynthetically active radiation (PAR).
 
 The sensor generates a uA current signal that is routed through a "Universal Transconductance Amplifier" (UTA) mfg by EME Systems.  http://www.emesystems.com/uta_dat.htm
 
 The amplifier converts signal from current to voltage and boosts the signal between 0 and 5 volts.
 Sketch converts from the analog-to-digital raw signal to volts using code from Section 5.9 of the Arduino Cookbook (Michael Margolis, 2011).
 Adapts a moving average algorithm found in "Environmental Monitoring With Arduino".  https://github.com/ejgertz/EMWA/blob/master/chapter-2/NoiseMonitor

 This sketch is particularly verbose in order to confirm calcs and do general debugging.

 
 Signal Conversion Notes:

 Program is designed specifically for a new LI-COR Quantum sensor purchased by the Barnard Ecohydrology Lab.  Serial number "Q 47952".
 The cert. of calibration (Jun 19, 2012) has a calibration constant of 6.97 microamps per 1000 umol s^-1 m^-2.
 This equates to 143.47 umol s^-1 m^-2 / microamp.
 The UTA has a "gain factor" of 0.4 V / uA when the signal is amplified and converted from current to voltage.

 Their data sheet says that, to convert the signal to PAR level, you need to use the formula:
   
      PAR level [umol s^-1 m^-2]  = UTA Output [Volts]  *  LI-COR Calibration Multiplier [umol s^-1 m^-2 / uA]
                                    ----------------------------------------------------------------------
                                                              UTA gain factor [V / uA]

 ... the units cancel out, so what could go wrong?
 
 Data Logging Notes:
 
 The sketch also makes use of several Serial.print statements that output data to an OpenLog SD card reader/writer (https://github.com/sparkfun/OpenLog)
 The OpenLog device takes the serial stream output and records it to a text file.  The Sparkfun Device is pretty simple, but caution is needed to make sure it 
 doesn't get a flood of random 0s and 1s when a sketch is uploaded to the device.  
 
 Remember to temporarily unplug Tx when a sketch is uploaded. 

Created by Matt Findley, July 1, 2012.  mcfindster@gmail.com.

*/

// pin settings
const int UTA_PIN = 0;        // use input pin A0 for the UTA output.

// variables for average PAR calculations. 
const int NUMBER_OF_SAMPLES = 30;     // create integer constant representing the number of samples that make up the average.
const int SAMPLE_FREQUENCY = 1000;    // create integer constant representing measurement frequency for making PAR measurments, in milliseconds.  
float PAR_Samples[NUMBER_OF_SAMPLES]; // create variable that is an array of single precision floating-point numbers 
                                      // (aka non-integer numbers with a fractional component) for storing PAR measurements.

float avgPAR;                         // create variable (floating number) that holds the average of the measurements
float sumOfSamples = 0;               // create variable (floating number) to keep running total of sum of the last 15 measurements.
int counter = 0;                      // integer variable used to loop (aka iterate) through the algorithm for creating the average measurement.

// constants for converting signal from raw data to PAR level
  const float REFERENCE_VOLTS = 5.0; // create variable (floating number) for the max voltage on the Arduino Uno's analog-to-digital (ADC) converter.
  // see intro to program for explanation for these "magic numbers":
  const float LICOR_CAL_MULTIPLIER = 143.47;
  const float UTA_GAIN_FACTOR = 0.40;

void setup()
{

  Serial.begin(9600);  // begin a serial output stream for sending data back to the computer.
  
  delay(5000);  // pause 5 sec before starting.

  Serial.println("count\told\tnew");  // send a string of text with the table header to the serial output stream.  
                                      // ... can be read from Serial Monitor in the Arduino.
                                      // the "\t" bit in the string is a tab character.
   
}

void loop()
{
  counter = ++ counter % NUMBER_OF_SAMPLES;  // modulo: returns the remainder after counter is divided by number of Samples
                                            // basically, for a sample size of 30, the counter loops between 0 and 29, resetting to 0 when counter reaches 30.
  Serial.print(counter);
  Serial.print("\t");
  Serial.print(PAR_Samples[counter]);
  Serial.print("\t");

  sumOfSamples = sumOfSamples - PAR_Samples[counter];  // subtract oldest sample from the running total.

  PAR_Samples[counter] = acquirePAR(); // swap old sample with a new one.

  Serial.println(PAR_Samples[counter]);

  sumOfSamples = sumOfSamples + PAR_Samples[counter];  // add the latest sample to running total.

  if(counter == 0){
    avgPAR = sumOfSamples / NUMBER_OF_SAMPLES;         // calc avgPAR only at the end of a 30 sample cycle.  ... not a moving average, but a simple avg.
    Serial.print("AvgPAR:\t\t");
    Serial.println(avgPAR);
    Serial.println();
  
    Serial.println("count\told\tnew");
  }
  delay(SAMPLE_FREQUENCY);
}

float acquirePAR(void)
{
  int val = analogRead(UTA_PIN);  // read the value from the sensor
                                  // should be int between 0 and 1023 (2^10 is a 10 bit ADC)
  float volts = val * REFERENCE_VOLTS / 1023;  // order of operations is important here, otherwise integer division takes place.
  float PAR_Level = volts * LICOR_CAL_MULTIPLIER / UTA_GAIN_FACTOR;  // calculate PAR level umol s^-1 m^-2
  return PAR_Level;
}
