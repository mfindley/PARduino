/* 
 Read the signal generated from a LI-COR Quantum PAR sensor and record it.  Sensor measures photosynthetically active radiation (PAR).
 
 The LI-COR sensor generates a uA current signal that is routed through a "Universal Transconductance Amplifier" (UTA) mfg by EME Systems.  http://www.emesystems.com/uta_dat.htm
 
 The amplifier converts signal from current to voltage and boosts the signal between 0 and 5 volts.
 Sketch converts from the analog-to-digital raw signal to volts using code from Section 5.9 of the Arduino Cookbook (Michael Margolis, 2011).

 Signal Conversion Notes:

 Program is designed specifically for a new LI-COR Quantum sensor purchased by the Barnard Ecohydrology Lab.  Serial number "Q 47952".
 The cert. of calibration (Jun 19, 2012) has a calibration constant of 6.97 microamps per 1000 umol s^-1 m^-2.
 This equates to 143.47 umol s^-1 m^-2 / microamp.
 The UTA has a "gain factor" of 0.3 V / uA when the signal is amplified and converted from current to voltage.  
 It was set at the factory to 0.4 V / uA, but this clipped off PAR levels at peak sunlight, so I reduced the gain factor by resetting switches.
 see: http://www.emesystems.com/pdfs/UTA_ver2A_120407.pdf for discussion of changing the gain.
 

 EME data sheet says that, to convert the signal to PAR level, you need to use the formula:
   
      PAR level [umol s^-1 m^-2]  = UTA Output [Volts]  *  LI-COR Calibration Multiplier [umol s^-1 m^-2 / uA]
                                    ----------------------------------------------------------------------
                                                              UTA gain factor [V / uA]

 ... the units cancel out, so what could go wrong?
 
 The LI-COR sensor and the UTA are connected to the circuit with three wires - a power line, a ground line, and a signal line.  
 The signal wire feeds into analog pin A0 on the Arduino.  Power and ground are provided by the 9V DC power source directly, because the voltage used to power the UTA must be at
 least 1 volt higher than the Arduino's 5 volt reference voltage.
 
 Data Logging Notes:
 
 The sketch makes use of several SoftwareSerial print statements that output data to an OpenLog SD card reader/writer (https://github.com/sparkfun/OpenLog)
 The OpenLog device takes the serial stream output and records it to a text file.  The Sparkfun Device is pretty simple, but caution is needed to make sure it 
 doesn't get a flood of random 0s and 1s when a sketch is uploaded to the device.  ... still not sure of the best work-flow to keep junk from writing to the SD card.
 Using a separate software serial connection for the openLog device instead of a shared line with the PC's USB cord seems to do the trick.  

 The program has the OpenLog's receiving line (Rx) connected to the Arduino's digital pin 5.  Pin 5 is used as the Tx pin from the Arduino.

Created by Matt Findley, July 1, 2012.  mcfindster@gmail.com.

Change log.

7/7/2012 - revised to record discrete data instead of averaging.  There seems to plenty of storage space on the SD card.
7/11/2012 - changed switches on the UTA to make the gain factor 0.3 instead of 0.4.  This will keep PAR from going off scale on the ADC during peak sunlight.
7/15/2012 - changed code to use an external interrupt to read data whenever the alarm on the real time clock triggers.
*/

#include <SPI.h>             // need this libary to communicate with the DS3234 Real Time Clock (RTC). 

#include <SoftwareSerial.h>  // this used to be the NewSoftSerial library that I used to use for early forays into creating data logging applications.
                             // The library is now included in the Arduino 1.0 IDE and has a new name.
                             // Basically, it allows the programmer to set up a serial data stream that is separate from the hardware based Tx/Rx pins
                             // (digital pins 0 and 1) that are on the Uno and are transmitted to the PC over the USB cable.
                             // The SoftwareSerial library allows for a separate stream going only to the data logger.  

// pin and flag settings:
const int UTA_PIN = 0;         // use analog input pin A0 to read  output from the UTA.

const int LOGGER_PIN = 5;      // pin used to transmit date, time, and PAR reading to the data logger.

const int CHIP_SELECT_PIN = 8; // digital pin 8 is used in the SPI communication protocol to let the RTC know who the boss is and when it needs to be communicating.
                               // the Arduino also uses dig pins 11, 12, and 13 for master output/slave input, master input/slave output, and the serial clock respectively.  
                               // see http://arduino.cc/en/Reference/SPI for more info.

const int RTC_INTERRUPT = 0;   // catching the RTC alarm using Interrupt0 on the microcontroller.                           
                               // Interrupt0 maps to digital pin 2 on the Arduino, which is a bit confusing.
   

volatile int interruptFlag;    // declare a variable to act as an interrupt flag.  
                               // See figure 1 http://www.ti.com/lit/an/slaa294a/slaa294a.pdf for conceptual model for using this flag.
                               // set up as volatile variable because we work with it in the interrupt service routine.

// variables for sampling data: 
float measuredPAR;                    // create variable (floating number) that holds the PAR measurement.
String measurementTimeDate;           // string to hold the date and time for the measurement.

// constants for converting the raw signal to a PAR value
const float REFERENCE_VOLTS = 5.0; // create variable (floating number) for the max voltage on the Arduino Uno's analog-to-digital (ADC) converter.
// see intro to program for explanation for the following "magic numbers":
const float LICOR_CAL_MULTIPLIER = 143.47;
const float UTA_GAIN_FACTOR = 0.30;

// create a software-based serial connection to send data to the OpenLog device.  ... uses the SoftwareSerial library shipping with the Arduino 1.0 IDE.
SoftwareSerial logger(0, LOGGER_PIN);  // creates an SoftwareSerial object called logger that transmits on the LOGGER_PIN
                                       // first argument is for the receiving pin - not used because communication is only one way.
                                       // the sketch only writes data to the logger.
void setup()
{
  Serial.begin(9600);
  logger.begin(9600);  // begin a serial output stream for sending data to the data logger.
  interruptFlag = 0;
  initializeRTC();
  Serial.println("Date\tTime\tPAR");  // table headers on the serial stream to the pc
  logger.println("Date\tTime\tPAR");  // same thing but to the data logger
  attachInterrupt(RTC_INTERRUPT, RealTimeClockInterruptServiceRoutine, FALLING);  // sets up interrupt0 to trigger whenever the RTC alarm falls from high to low.
}

void loop()
{
  if(interruptFlag == 1){
    measurementTimeDate = readTime();    // calls function that reads the time and date off the real time clock.
    measuredPAR = acquirePAR();          // calls function that reads voltage on Analog pin 0 and converts it to PAR level.

    Serial.print(measurementTimeDate);
    Serial.print("\t");
    Serial.println(measuredPAR);
  
    logger.print(measurementTimeDate);
    logger.print("\t");
    logger.println(measuredPAR);

    clearInterruptFlag();
  }
}

///////////////////////////////////////
void RealTimeClockInterruptServiceRoutine()
{
  interruptFlag = 1;
}

///////////////////////////////////////
void clearInterruptFlag(){
  interruptFlag = 0;          // clears interrupt flag on the Arduino
  // clears interrupt flag on the RTC:
  digitalWrite(CHIP_SELECT_PIN, LOW);      // pull the chip select pin connected to the RTC low, so that the RTC knows that the Arduino wants to communicate.
  SPI.transfer(0x8F);         // address for the RTC's control register. 
  SPI.transfer(B00000000);    // blunt way to clear the alarm2 flag (and everything else in the register too, but that doesn't really matter).
  digitalWrite(CHIP_SELECT_PIN, HIGH);     // pull the chip select pin connected to the RTC high, so that the RTC knows that we're done talking.
  delay(100);
}

//////////////////////////////////////
float acquirePAR(void)  // thisfunction captures data from the LI-COR Sensor.
{
  int val = analogRead(UTA_PIN);  // read the value from the sensor
                                  // should be int between 0 and 1023 (2^10 is a 10 bit ADC)
  float volts = val * REFERENCE_VOLTS / 1023;  // order of operations is important here, otherwise integer division takes place.
  float PAR_Level = volts * LICOR_CAL_MULTIPLIER / UTA_GAIN_FACTOR;  // calculate PAR level umol s^-1 m^-2
  return PAR_Level;               // sends the measured PAR level back to the part of the main program that called this function.
}                                 // end bracket for the acquirePAR function.


//////////////////////////////////////
String readTime() {  // reaad time and date off the clock.
  String temp;
  int TimeDate [7]; //second,minute,hour,day-of-week,day,month,year		
  for(int i=0; i<=6;i++){
    if(i==3){  // skip day of week
      i++;
      }
    digitalWrite(CHIP_SELECT_PIN, LOW);
    SPI.transfer(i+0x00); 
    unsigned int n = SPI.transfer(0x00);        
    digitalWrite(CHIP_SELECT_PIN, HIGH);
    int a=n & B00001111;    
    if(i==2){	
      int b=(n & B00110000)>>4; //24 hour mode
      if(b==B00000010)
        b=20;        
      else if(b==B00000001)
      b=10;
    TimeDate[i]=a+b;
    }
    else if(i==4){
      int b=(n & B00110000)>>4;
      TimeDate[i]=a+b*10;
    }
    else if(i==5){
      int b=(n & B00010000)>>4;
      TimeDate[i]=a+b*10;
    }
    else if(i==6){
     int b=(n & B11110000)>>4;
     TimeDate[i]=a+b*10;
    }
    else{	
      int b=(n & B01110000)>>4;
      TimeDate[i]=a+b*10;	
    }
  }
  // assemble the time/date string
  temp.concat(TimeDate[5]);  // month goes first according to US convention.
  temp.concat("/") ;         // slashes separate the month/day/year
  temp.concat(TimeDate[4]);  // now day of month.
  temp.concat("/") ;
  temp.concat(TimeDate[6]);  // now year.
  temp.concat("\t") ;     // now tab to separate the date from the time.
  temp.concat(TimeDate[2]);  // now hour.
  temp.concat(":") ;         // colons separate the hours:minutes:seconds
  if(TimeDate[1] < 10)
    temp.concat("0");        // puts a leading zero in front of minutes between 0 and 9.
  temp.concat(TimeDate[1]);
  temp.concat(":") ;
  if(TimeDate[0] < 10)
    temp.concat("0");        // puts a leading zero in front of seconds between 0 and 9.
    temp.concat(TimeDate[0]);
  return(temp);
}

//////////////////////////////////////
void initializeRTC() {
  SPI.begin();  // start command for SPI.  used in Sparkfun's example code
  SPI.setBitOrder(MSBFIRST);  // configuration command used to communicate with the clock.
  SPI.setDataMode(SPI_MODE3); // both mode 1 & 3 should work 
                              // despite sparkfun's above comment, I was only able to get it work in Mode 3, the this part of the code is slightly different.

 //configure the Alarm 2 control registers on the RTC to fire an interrupt on 00 seconds of every minute - see table 2 of datasheet.
  pinMode(CHIP_SELECT_PIN, OUTPUT);        // configure the chip select pin as an output pin 
  digitalWrite(CHIP_SELECT_PIN, LOW);      // pull the chip select pin connected to the RTC low, so that the RTC knows that the Arduino wants to communicate with it.


  SPI.transfer(0x8B);         // send a command which contains the register address that we want to write to.  
                              // 0x8B is the first register in the series of alarm2 programming registers
                              // the following commands set alarm2 to trigger at 00 seconds of every minute
                              // see table 2 of the RTC's data sheet http://www.sparkfun.com/datasheets/BreakoutBoards/DS3234.pdf

  SPI.transfer(B10000000);   // set mask for seconds
  SPI.transfer(B10000000);   // set mask for hour
  SPI.transfer(B10000000);   // set mask for day/date
  
  digitalWrite(CHIP_SELECT_PIN, HIGH);      // pull the chip select pin connected to the RTC high, so that the RTC knows that we're done talking.
 
  delay(100);
  
  digitalWrite(CHIP_SELECT_PIN, LOW);      // pull the chip select pin connected to the RTC low, so that the RTC knows that the Arduino wants to communicate.
  SPI.transfer(0x8E);         // address for the RTC's control register. 
  SPI.transfer(B00000110);   // set the interupt control and enable alarm 2.
  SPI.transfer(B00000000);   // set OSF to zero, disable the battery enabled 32 kHz signal, disable the 32 kHz signal.

  digitalWrite(CHIP_SELECT_PIN, HIGH);      // pull the chip select pin connected to the RTC high, so that the RTC knows that we're done talking.
 
  delay(100);
}

