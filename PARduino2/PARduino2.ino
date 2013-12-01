/* 

PARduino measures and logs photosynthetically active radiation (PAR). 

It was designed for the Barnard Ecohydology Lab at the University of Colorado, Boulder  (http://instaar.colorado.edu/research/labs-groups/ecohydrology-laboratory/).

The program is designed for the following components:
 - an Arduino Pro Mini (8 mHz / 3.3 volts)  https://www.sparkfun.com/products/11114?
 - DeadOn Real Time Clock - DS3234 Breakout Board https://www.sparkfun.com/products/10160
 - microSD card breakout board https://www.sparkfun.com/products/544?
 - EME Systems Universal Transconductance Amplifier (UTA) w/ BNC connector http://www.emesystems.com/uta_dat.htm  http://www.emesystems.com/pdfs/UTA_ver2A_120407.pdf 
 - LI-COR Quantum PAR sensor http://www.licor.com/env/pdf/light/190.pdf (Li-190SA version with a BNC connector)

Created using version 1.0.1 of the Arduino IDE.

Program notes:

  The microcontroller sleeps in low power mode when not acquiring data.  
  This is based loosely on the example at http://www.engblaze.com/hush-little-microprocessor-avr-and-arduino-sleep-mode-basics/ ,
  but modified to wake on an external interrupt triggered when the RTC alarms on the minute at 00 seconds.

  Configuring and formatting the clock data is based on the Sparkfun example at http://dlnmh9ip6v2uc.cloudfront.net/datasheets/BreakoutBoards/DS3234_Example_Code.pde,
  but modified to set the alarm, set the interrupt, and to read time/date using a multi-byte read on the DS3234 real time clock.
   ... wanted to read the clock using multi-byte read because page 11  of the data sheet, 
   http://www.sparkfun.com/datasheets/BreakoutBoards/DS3234.pdf, suggests that this keeps registers from updating during a read cycle. 

  Initalizing of the SD card and writing to the file is based on an example, ReadWriteSdFat, at http://code.google.com/p/sdfatlib/

Dependencies: 
  Except for the SdFat library,  all of the software libaries used by this code are included in the Arduino 1.0.1 IDE.
  This program uses the July 2012 SdFat library at http://code.google.com/p/sdfatlib/

Signal Conversion Notes:

 Program is designed specifically for a new LI-COR Quantum sensor purchased by the Barnard Ecohydrology Lab.  Serial number "Q 47952".
 The cert. of calibration (Jun 19, 2012) has a calibration constant of 6.97 microamps per 1000 umol s^-1 m^-2.
 This equates to 143.47 umol s^-1 m^-2 / microamp.
 The UTA was set to have a "gain factor" of 0.2 V / uA when the signal is amplified and converted from current to voltage.

 The EME data sheet says that, to convert the signal to PAR level, you need to use the formula:
   
      PAR level [umol s^-1 m^-2]  = UTA Output [Volts]  *  LI-COR Calibration Multiplier [umol s^-1 m^-2 / uA]
                                    ----------------------------------------------------------------------
                                                              UTA gain factor [V / uA]

 ... the units cancel out, so what could go wrong?
 
 The LI-COR sensor and the UTA are connected to the Arduino with three wires - a power line, a ground line, and a signal line.  
 The signal wire feeds into pin A0. The supply voltage to the UTA must be at least 5 volts and must be 1 volt greater than the full scale output of the amplifier.
 
Circuit Notes:  

  pin map:
  
  RAW o-----o 9V wall wart barrel jack, red wire for UTA power.
  GND o-----o 9V wall wart barrel jack, black ground wire for UTA power, GND wires for RTC and SD card.
  VCC o-----o VCC to SD card and the RTC.
  
  A2  o-----o white signal wire from the UTA
  D2  o-----o interrupt (INT) pin on RTC break out board (BoB)
  D5  o-----o card detect (CD) on SD BoB (... not really used in this sketch)
  D8  o-----o chip select (CS) on RTC.  used for SPI communication.
  D10 o-----o chip select (CS) for the SD card.  used for SPI communication.
  D11 o-----o master out slave in (MOSI) line for the RTC and the SD card.  used for SPI communication.
  D12 o-----o master in slave out (MISO) line for the RTC and the SD card.  used for SPI communication.
  D13 o-----o clock for SPI communication. connected to CLK on the RTC and the SD card.

  Design documents for a printed circuit board (PCB) connecting all these components are available at the
  PARduino github repository.

License:

  This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 United States License. 
  To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/us/ or 
  send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

  Created by Matt Findley, Boulder, Colorado, Sep 1, 2012.  matthewfindley@hotmail.com.

Change log:  

9/1/2012 - added annotation.
6/28/2013 - changed license for arduino code to Creative Commons Attribution-ShareAlike.
          - documented library dependencies.
          - provided reference to PCB in the PARduino GitHub repository.
7/20/2013 - provided more text info written in the header of the data file during initialization.
          - now includes PAR sensor serial number, calibration and gail factors (hard coded in - it does read the variables)
          - time zone (hard coded as mountain standard time, but the code has no way to check this.)
10/20/2013- use card detect switch to decide whether to initiate a write to the card or not.
*/

#include <SPI.h>  // used to communicate with the RTC and the SD card peripherals.
#include <SdFat.h>  // library for working with files on the SD card.
// these libraries are used to move in and out of low power mode:
#include <avr/interrupt.h>  
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/io.h>

SdFat sd;  // used by the SDFat library to create an C++ object representing the SD card
SdFile myFile;  // used by the SDFat library to create an C++ object representing the data file

const int RTC_CS=8; //chip select pin for real time clock
const int SD_CS=10; //chip select pin for SD card
const int UTA_PIN=A2; // reads the LiCOR through the UTA using analog pin 2
const int RTC_INTERRUPT=0;   // catching the RTC alarm using Interrupt0 on the microcontroller.                           
                             // Interrupt0 maps to digital pin 2 on the Arduino and is variously identifed by pin number or by interrupt number, which is a bit confusing.

volatile int interruptFlag = 0;    // declare a variable to act as an interrupt flag.  
                               // set up as volatile variable because we work with it in the interrupt service routine.

// constants for converting the raw signal to a PAR value
const float REFERENCE_VOLTS = 3.3; // create variable (floating number) for the max voltage on the Arduino's analog-to-digital (ADC) converter.
// see intro to program for explanation for the following "magic numbers":
const float LICOR_CAL_MULTIPLIER = 143.47;
const float UTA_GAIN_FACTOR = 0.20;
   
// variables for sampling data: 
float measuredPAR;                    // create variable (floating number) that holds the PAR measurement.
String measurementTimeDate;           // string to hold the date and time for the measurement.


void setup(void){
  Serial.begin(9600);
  pinMode(RTC_CS, OUTPUT);  // set the chip select pin to out before anything else!!!
  pinMode(SD_CS, OUTPUT);  // set the chip select pin to out before anything else!!!
  pinMode(5, INPUT_PULLUP); // set card detect pin as digital input with interal pull up resistor
  initializeRTC();   // set alarm2 and enable interrupt on the DS3234
  attachInterrupt(RTC_INTERRUPT, RealTimeClockInterruptServiceRoutine, FALLING);  // sets up interrupt0 to trigger whenever the RTC alarm falls from high to low.

  int cardPresent = digitalRead(5);

  if (cardPresent == LOW) // if an SDCard is detected in the socket.
  {
    Serial.println("Card in Socket");

    // write header to SD card  
    if (!sd.begin(SD_CS, SPI_HALF_SPEED)) sd.initErrorHalt();

    // open the file for write at end like the Native SD library
    if (!myFile.open("data.txt", O_RDWR | O_CREAT | O_AT_END)) {
      sd.errorHalt("opening data.txt for write failed");
    }
    // if the file opened okay, write to it:
    Serial.print("#Header Text... Initialized:  ");
    Serial.println(readTime());

    myFile.print("#Header Text... Initialized:  ");
    myFile.println(readTime());

    myFile.println("#PAR Sensor Model: LI190  Serial Number: Q47952");
    myFile.println("#Calibration Constant: 143.47 micromol s-1 m-2 / microamp");
    myFile.println("#Amplifier Gain: 0.2 Volt / microamp");
    myFile.println("#Time Zone:  Mountain Standard Time");

    Serial.println("#PAR Sensor Model: LI190  Serial Number: Q47952");
    Serial.println("#Calibration Constant: 143.47 micromol s-1 m-2 / microamp");
    Serial.println("#Amplifier Gain: 0.2 Volt / microamp");
    Serial.println("#Time Zone:  Mountain Standard Time");


    // close the file:
    myFile.close();
    Serial.println("done.");
  }
  else
  {
    Serial.println("Card not in Socket");
  }
}

void loop(void){
  sleepNow();
  if(interruptFlag == 1){
    measurementTimeDate = readTime();    // calls function that reads the time and date off the real time clock.
    measuredPAR = acquirePAR();          // calls function that reads voltage on Analog pin 0 and converts it to PAR level.

//    Serial.print(measurementTimeDate);
//    Serial.print("\t");
//    Serial.println(measuredPAR);
  

  if (digitalRead(5) == LOW) // if an SDCard is detected in the socket.
  {
    if (!sd.begin(SD_CS, SPI_HALF_SPEED)) sd.initErrorHalt();

    // open the file for write at end like the Native SD library
    if (!myFile.open("data.txt", O_RDWR | O_CREAT | O_AT_END)) {
      sd.errorHalt("opening data.txt for write failed");
    }
    // if the file opened okay, write to it:
    //  Serial.println(readTime());
    myFile.print(readTime());
    myFile.print("\t");
    myFile.println(measuredPAR);
    // close the file:
    myFile.close();
    //  Serial.println("done.");
  }
    clearInterruptFlag();
  }
  
}

void initializeRTC() {
  SPI.begin();  // start command for SPI.  used in Sparkfun's example code
  SPI.setBitOrder(MSBFIRST);  // configuration command used to communicate with the clock.
  SPI.setDataMode(SPI_MODE3); // both mode 1 & 3 should work 
                              // despite sparkfun's above comment, I was only able to get it work in Mode 3, the this part of the code is slightly different.

 //configure the Alarm 2 control registers on the RTC to fire an interrupt on 00 seconds of every minute - see table 2 of DS3234 datasheet.
  digitalWrite(RTC_CS, LOW);      // pull the chip select pin connected to the RTC low, so that the RTC knows that the Arduino wants to communicate with it.


  SPI.transfer(0x8B);         // send a command which contains the register address that we want to write to.  
                              // 0x8B is the first register in the series of alarm2 programming registers
                              // the following commands set alarm2 to trigger at 00 seconds of every minute
                              // see table 2 of the RTC's data sheet http://www.sparkfun.com/datasheets/BreakoutBoards/DS3234.pdf

  SPI.transfer(B10000000);   // set mask for seconds
  SPI.transfer(B10000000);   // set mask for hour
  SPI.transfer(B10000000);   // set mask for day/date
  
  digitalWrite(RTC_CS, HIGH);      // pull the chip select pin connected to the RTC high, so that the RTC knows that we're done talking.
 
  delay(10);
  
  digitalWrite(RTC_CS, LOW);      // pull the chip select pin connected to the RTC low, so that the RTC knows that the Arduino wants to communicate.
  SPI.transfer(0x8E);         // address for the RTC's control register. 
  SPI.transfer(B00000110);   // set the interupt control and enable alarm 2.
  SPI.transfer(B00000000);   // set OSF to zero, disable the battery enabled 32 kHz signal, disable the 32 kHz signal.

  digitalWrite(RTC_CS, HIGH);      // pull the chip select pin connected to the RTC high, so that the RTC knows that we're done talking.
 
  delay(10);
  SPI.end();  // added for symmetry.

}

String readTime(){
  // read time:
  SPI.begin();
  SPI.setBitOrder(MSBFIRST); 
  SPI.setDataMode(SPI_MODE3);
  delay(10);
  String temp;
  unsigned int timeDateArray[7];  // sec, min, hr, dow, dy, mo, yr
  digitalWrite(RTC_CS, LOW);  // Tell RTC that we're talkin' to it.
  SPI.transfer(0x00);  // tell the RTC that we want to read from
                       // first byte in its register.  This holds
                       // seconds.

  /* the following code loops and reads the contents of
     the first seven registers on the RTC.  SPI automatically
     goes to the next register during reads and writes, so we
     don't have to increment the register in this code - we just
     keep reading... */
  for(int i=0; i<=6; i++){
    unsigned int n = SPI.transfer(0x00);  // here, the SPI.transfer cmd means we are just reading data.

    /* extract the four least significant bits (LSb) from the raw data.  The four most significant bits (MSb)
       are treated slightly different for registers holding hours and month, so we will process those bits in
       in the conditional statements. */

    int a = n & B00001111;  // the AND operator will zero out the most significant bits. 

    /* following conditional statements are adopted from Sparkfun example code: 
    http://dlnmh9ip6v2uc.cloudfront.net/datasheets/BreakoutBoards/DS3234_Example_Code.pde  
    and are used to turn the register bits into more meaningful values for time and date elements*/
    
    if(i==2){	// if we're dealing with the hour part of the time...
     int b=(n & B00110000)>>4; //24 hour mode
     if(b==B00000010)
	b=20;        
     else if(b==B00000001)
	b=10;
     timeDateArray[i]=a+b;
    }
    else if(i==3){  // if we're dealing with the day-of-week part of the time...
     timeDateArray[i]=a;
    }
    else if(i==4){  // if we're dealing with the date part of the time...
     int b=(n & B00110000)>>4;
     timeDateArray[i]=a+b*10;
    }
    else if(i==5){   // if we're dealing with the month part of the time...
     int b=(n & B00010000)>>4;
     timeDateArray[i]=a+b*10;
    }
    else if(i==6){  // if we're dealing with the year part of the time...
     int b=(n & B11110000)>>4;
     timeDateArray[i]=a+b*10;
    }
    else{	// pretty much only seconds and minutes are left and they are treated the same way.
     int b=(n & B01110000)>>4;
     timeDateArray[i]=a+b*10;	
    }
    
  }  // end the FOR loop for the SPI multibyte-read of the RTC

  digitalWrite(RTC_CS, HIGH);  // Tell the RTC that we're done talking.
  SPI.end();  // not sure this is needed, but provides symmetry with the SPI.begin statement.
 
  /* Format time:  Now we print the raw bit patterns pulled from the RTC */
//  Serial.println("Here's the raw RTC time data:");
//  for(int i=0; i<=6; i++){
//    Serial.println(timeDateArray[i]);
//  }

  /* Now we print a text string that is more comprehendable.*/
  temp.concat(timeDateArray[5]);  // month goes first according to US convention.
  temp.concat("/") ;         // slashes separate the month/day/year
  temp.concat(timeDateArray[4]);  // now day of month.
  temp.concat("/20") ;       // let's present the date in YYYY format.  This code should be OK for the next 88 years.
  temp.concat(timeDateArray[6]);  // now the last two digits of the year.
  temp.concat("\t") ;        // now tab to separate the date from the time.
  temp.concat(timeDateArray[2]);  // now hour.
  temp.concat(":") ;         // colons separate the hours:minutes:seconds
  if(timeDateArray[1] < 10)
    temp.concat("0");        // puts a leading zero in front of minutes between 0 and 9.
  temp.concat(timeDateArray[1]);
  temp.concat(":") ;
  if(timeDateArray[0] < 10)
    temp.concat("0");        // puts a leading zero in front of seconds between 0 and 9.
    temp.concat(timeDateArray[0]);
  return(temp);
}

void clearInterruptFlag(){
  SPI.begin();
  SPI.setBitOrder(MSBFIRST); 
  SPI.setDataMode(SPI_MODE3);
  delay(10);
  interruptFlag = 0;          // clears interrupt flag on the Arduino
  // clears interrupt flag on the RTC:
  digitalWrite(RTC_CS, LOW);      // pull the chip select pin connected to the RTC low, so that the RTC knows that the Arduino wants to communicate.
  SPI.transfer(0x8F);         // address for the RTC's control register. 
  SPI.transfer(B00000000);    // blunt way to clear the alarm2 flag (and everything else in the register too, but that doesn't really matter).
  digitalWrite(RTC_CS, HIGH);     // pull the chip select pin connected to the RTC high, so that the RTC knows that we're done talking.
  delay(10);
  SPI.end();
}

void RealTimeClockInterruptServiceRoutine()
{
  interruptFlag = 1;
}

void sleepNow(void)
{
    delay(100);
    //
    // Choose our preferred sleep mode:
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    //
    // Set sleep enable (SE) bit:
    sleep_enable();
    //
    // Put the device to sleep:
    sleep_mode();
    //
    // Upon waking up, sketch continues from this point.
    sleep_disable();
}

float acquirePAR(void)  // thisfunction captures data from the LI-COR Sensor.
{
  int val = analogRead(UTA_PIN);  // read the value from the sensor
                                  // should be int between 0 and 1023 (2^10 is a 10 bit ADC)
  float volts = val * REFERENCE_VOLTS / 1023;  // order of operations is important here, otherwise integer division takes place.
  float PAR_Level = volts * LICOR_CAL_MULTIPLIER / UTA_GAIN_FACTOR;  // calculate PAR level umol s^-1 m^-2
  return PAR_Level;               // sends the measured PAR level back to the part of the main program that called this function.
}                                 // end bracket for the acquirePAR function.


