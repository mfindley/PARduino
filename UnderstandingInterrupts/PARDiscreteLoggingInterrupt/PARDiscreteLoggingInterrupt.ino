/*
 Discrete PAR Logging
 
 Read the signal generated from a LI-COR Quantum PAR sensor and record it - no averaging.  Sensor measures photosynthetically active radiation (PAR).
 
 The sensor generates a uA current signal that is routed through a "Universal Transconductance Amplifier" (UTA) mfg by EME Systems.  http://www.emesystems.com/uta_dat.htm
 
 The amplifier converts signal from current to voltage and boosts the signal between 0 and 5 volts.
 Sketch converts from the analog-to-digital raw signal to volts using code from Section 5.9 of the Arduino Cookbook (Michael Margolis, 2011).

 Signal Conversion Notes:

 Program is designed specifically for a new LI-COR Quantum sensor purchased by the Barnard Ecohydrology Lab.  Serial number "Q 47952".
 The cert. of calibration (Jun 19, 2012) has a calibration constant of 6.97 microamps per 1000 umol s^-1 m^-2.
 This equates to 143.47 umol s^-1 m^-2 / microamp.
 The UTA has a "gain factor" of 0.3 V / uA when the signal is amplified and converted from current to voltage.

 EME data sheet says that, to convert the signal to PAR level, you need to use the formula:
   
      PAR level [umol s^-1 m^-2]  = UTA Output [Volts]  *  LI-COR Calibration Multiplier [umol s^-1 m^-2 / uA]
                                    ----------------------------------------------------------------------
                                                              UTA gain factor [V / uA]

 ... the units cancel out, so what could go wrong?
 
 The LI-COR sensor and the UTA are connected to the Arduino with three wires - a power line (>reference voltage + 1V!), a ground line, and a signal line.  
 The signal wire feeds into pin A0.
 
 Data Logging Notes:
 
 The sketch also makes use of several SoftwareSerial print statements that output data to an OpenLog SD card reader/writer (https://github.com/sparkfun/OpenLog)
 The OpenLog device takes the serial stream output and records it to a text file.  The Sparkfun Device is pretty simple, but caution is needed to make sure it 
 doesn't get a flood of random 0s and 1s when a sketch is uploaded to the device.  ... still not sure of the best work-flow to keep junk from writing to the SD card.
 Using a separate software serial connection for the openLog device instead of a shared line with the PC's USB cord seems to help.  

 The program has the OpenLog's receiving line (Rx) connected to the Arduino's digital pin 5.  Pin 5 is used as the Tx pin from the Arduino.

Created by Matt Findley, July 1, 2012.  mcfindster@gmail.com.

Change log.

7/7/2012 - revised to record discrete data instead of averaging.  There seems to plenty of storage space on the SD card.
7/11/2012 - changed switches on the UTA to make the gain factor 0.3 instead of 0.4.  This will keep from going off scale on the ADC during peak sunlight.
*/

#include <SoftwareSerial.h>  // this used to be the NewSoftSerial library that I used to use for early forays into creating data logging applications.
                             // it now ships with the Arduino 1.0 IDE and has a new name.
                             // Basically, it allows the programmer to set up a serial data stream that is separate from the hardware based Tx/Rx pins
                             // (digital pins 0 and 1) that are on the Uno and are transmitted to the PC over the USB cable.
                             // The SoftwareSerial library allows for a separate stream going only to the data logger.  

#include <SPI.h>             // need this libary to communicate with the DS3234 Real Time Clock (RTC). 

// pin settings
const int UTA_PIN = 0;         // use input pin A0 for the UTA output.
const int LOGGER_PIN = 5;      // pin used to transmit PAR level to the data logger.
const int CHIP_SELECT_PIN = 8;   // digital pin 8 is used in the SPI communication protocol to let the RTC know who the boss is and when it needs to be communicating.
                               // the Arduino also uses dig pins 11, 12, and 13 for master output/slave input, master input/slave output, and the serial clock respectively.  
                               // see http://arduino.cc/en/Reference/SPI for more info.

const int INTERRUPT_PIN = 3;   // digital pin 3 used to catch the interrupt flag coming off of the RTC                           
                               //  confusing, but this is also known as interrupt 1  
                               
// variables for sampling rate. 
const int SAMPLE_FREQUENCY = 30000;    // create integer constant representing measurement frequency for making PAR measurments, in milliseconds. 
                                       // 30,000 milliseconds is sample collected every 30 seconds.

float measuredPAR;                    // create variable (floating number) that holds the PAR measurement.
String measurementTimeDate;           // string to hold the date and time for the measurement.

volatile boolean interruptFlag = false;

// constants for converting signal from raw data to PAR level
const float REFERENCE_VOLTS = 5.0; // create variable (floating number) for the max voltage on the Arduino Uno's analog-to-digital (ADC) converter.
// see intro to program for explanation for the following "magic numbers":
const float LICOR_CAL_MULTIPLIER = 143.47;
const float UTA_GAIN_FACTOR = 0.30;

// create a software-based serial connection to send data to the OpenLog device.  ... uses the SoftwareSerial library shipping with the Arduino 1.0 IDE.

SoftwareSerial logger(0, LOGGER_PIN);  // creates an SoftwareSerial object called logger that transmits on the LOGGER_PIN  

void setup()
{
  Serial.begin(9600);  // begin a serial output stream for sending data back to the computer.
  logger.begin(9600);  // begin a serial output stream for sending data to the data logger.
  RTC_init();                        // initialize the SPI communications protocol with the real time clock.

  Serial.println("Date\tTime\t PAR");
  logger.println("Date\tTime\t PAR");
  
  attachInterrupt(1, grabData, FALLING);
}

void loop()
{
  while(interruptFlag == true) {
    Serial.println("Do Something!);
    interruptFlag = false;
  }
  
}                                                      // end bracket for the Arduino's main loop method. 


//=====================================

void grabData(){

  interruptFlag = true;
  
}

//=====================================

float acquirePAR(void)                                 // this is a custom function used to capture data from the LI-COR Sensor.
{
  int val = analogRead(UTA_PIN);  // read the value from the sensor
                                  // should be int between 0 and 1023 (2^10 is a 10 bit ADC)
  float volts = val * REFERENCE_VOLTS / 1023;  // order of operations is important here, otherwise integer division takes place.
  float PAR_Level = volts * LICOR_CAL_MULTIPLIER / UTA_GAIN_FACTOR;  // calculate PAR level umol s^-1 m^-2
  return PAR_Level;               // sends the measured PAR level back to the part of the main program that called this function.
}                                 // end bracket for the acquirePAR function.

//=====================================

int RTC_init(){    // function is copied from DS3234 example code @ http://dlnmh9ip6v2uc.cloudfront.net/datasheets/BreakoutBoards/DS3234_Example_Code.pde
	  pinMode(CHIP_SELECT_PIN,OUTPUT); // chip select
	  // start the SPI library:
	  SPI.begin();
	  SPI.setBitOrder(MSBFIRST); 
	  SPI.setDataMode(SPI_MODE3); // mode 3 works 
}

//=====================================

String ReadTimeDate(){ // function is copied from DS3234 example code @ http://dlnmh9ip6v2uc.cloudfront.net/datasheets/BreakoutBoards/DS3234_Example_Code.pde
                       // not really important for this application, but array element [3] can be used for day of week with minor tweaks.
	String temp;
	int TimeDate [7]; //second,minute,hour,null,day,month,year		
	for(int i=0; i<=6;i++){
		if(i==3)
			i++;
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
	temp.concat("\t") ;     // now space to separate the date from the time.
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
