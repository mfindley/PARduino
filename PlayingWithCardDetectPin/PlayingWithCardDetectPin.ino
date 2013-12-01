


/*
 Card Detect / Input Pullup Serial
 
 This example demonstrates the use of pinMode(INPUT_PULLUP). It reads a 
 digital input on pin 5 and prints the results to the serial monitor.
 
 pin 5 is connected to a card detect switch on a microSD socket breakout board.
 
 The circuit: 
 * card detect swith attached from pin 5 to ground. 
 * Built-in LED on pin 13
 
 Unlike pinMode(INPUT), there is no pull-down resistor necessary. An internal 
 20K-ohm resistor is pulled to 5V. This configuration causes the input to 
 read HIGH when the card is in (switch is open), and LOW when the card is out
 (the switch is closed). 
 
 created 14 March 2012
 by Scott Fitzgerald
 
 modified by Matt Findley from:
 
 http://www.arduino.cc/en/Tutorial/InputPullupSerial
 
 This example code is in the public domain
 
 */
#include <SdFat.h>
#include <SPI.h>

const int cardDetectPin = 5;
const int RTC_CS=8; //chip select pin for real time clock
const int SD_CS=10; //chip select pin for SD card
const int UTA_PIN=A2; // reads the LiCOR through the UTA using analog pin 2
// constants for converting the raw signal to a PAR value
const float REFERENCE_VOLTS = 3.3; // create variable (floating number) for the max voltage on the Arduino's analog-to-digital (ADC) converter.
// see intro to program for explanation for the following "magic numbers":
const float LICOR_CAL_MULTIPLIER = 143.47;
const float UTA_GAIN_FACTOR = 0.20;
// variables for sampling data: 
float measuredPAR;                    // create variable (floating number) that holds the PAR measurement.
String measurementTimeDate;           // string to hold the date and time for the measurement.
int cardPresent;
SdFat sd;
SdFile myFile;

void setup()
{
  //start serial connection
  Serial.begin(9600);
  pinMode(RTC_CS, OUTPUT);  // set the chip select pin to out before anything else!!!
  pinMode(SD_CS, OUTPUT);  // set the chip select pin to out before anything else!!!
  pinMode(cardDetectPin, INPUT_PULLUP);   //set pin2 as an input w/ internal pull-up resistor

  initializeRTC();   // set alarm2 and enable interrupt on the DS3234
  cardPresent = digitalRead(cardDetectPin);
  while (cardPresent == LOW)   
  {
    // hang in this loop until the SD card is detected in the socket.
    Serial.println("Card Not Present");
    cardPresent = digitalRead(cardDetectPin);
    delay(500);
  }
  Serial.println("Card Present.  Leaving card detect loop");

  Serial.println("Initializing SD card in setup block...");
  if (!sd.begin(SD_CS, SPI_HALF_SPEED)) sd.initErrorHalt();

  // open the file for write at end like the Native SD library
  if (!myFile.open("PARData.txt", O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("opening PARData.txt for write failed");
  }
  // if the file opened okay, write to it:
  Serial.print("Writing to PARData.txt...");
  myFile.print("#Header Text... Initialized:  ");
  myFile.println(readTime());
  myFile.println("#PAR Sensor Model: LI190  Serial Number: Q47952");
  myFile.println("#Calibration Constant: 143.47 micromol s-1 m-2 / microamp");
  myFile.println("#Amplifier Gain: 0.2 Volt / microamp");
  myFile.println("#Time Zone:  Mountain Standard Time");
  // close the file:
  myFile.close();
  Serial.println("done.");

  // re-open the file for reading:
  if (!myFile.open("PARData.txt", O_READ)) {
    sd.errorHalt("opening PARData.txt for read failed");
  }
  Serial.println("PARData.txt:");

  // read from the file until there's nothing else in it:
  int data;
  while ((data = myFile.read()) >= 0) Serial.write(data);
  // close the file:
  myFile.close();
  Serial.println("card initialized.");
}

void loop()
{
  delay(3000); 
  //read the pushbutton value into a variable
  cardPresent = digitalRead(cardDetectPin);
  //print out the value of the pushbutton
  if (cardPresent == HIGH) 
  {
    Serial.println("Card Present");
    measurementTimeDate = readTime();    // calls function that reads the time and date off the real time clock.
    Serial.println("Time has been read");
    measuredPAR = acquirePAR();          // calls function that reads voltage on Analog pin 0 and converts it to PAR level.
    Serial.println("PAR has been read");

    if (!myFile.open("PARData.txt", O_RDWR | O_CREAT | O_AT_END)) {
      sd.errorHalt("opening test.txt for write failed");
    }
    // if the file opened okay, write to it:
    Serial.print("Writing to PARData.txt...");
    myFile.print(readTime());
    myFile.print("\t");
    myFile.println(acquirePAR());
    // close the file:
    myFile.close();
    Serial.println("done writing in loop.");

    // re-open the file for reading:
    if (!myFile.open("PARData.txt", O_READ)) {
      sd.errorHalt("opening PARData.txt for read failed");
    }
    Serial.println("PARData.txt:");
    // read from the file until there's nothing else in it:
    int data;
    while ((data = myFile.read()) >= 0) Serial.write(data);
    // close the file:
    myFile.close();

  } 
  else 
  {
    Serial.println("Card Not Present");
  }
}

void initializeRTC()
{
  SPI.begin();  // start command for SPI.  used in Sparkfun's example code
  SPI.setBitOrder(MSBFIRST);  // configuration command used to communicate with the clock.
  SPI.setDataMode(SPI_MODE3); 
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

String readTime()
{
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

float acquirePAR(void)  // this function captures data from the LI-COR Sensor.
{
  int val = analogRead(UTA_PIN);  // read the value from the sensor
                                  // should be int between 0 and 1023 (2^10 is a 10 bit ADC)
  float volts = val * REFERENCE_VOLTS / 1023;  // order of operations is important here, otherwise integer division takes place.
  float PAR_Level = volts * LICOR_CAL_MULTIPLIER / UTA_GAIN_FACTOR;  // calculate PAR level umol s^-1 m^-2
  return PAR_Level;               // sends the measured PAR level back to the part of the main program that called this function.
}                                 // end bracket for the acquirePAR function.


